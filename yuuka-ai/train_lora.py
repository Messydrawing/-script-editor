"""Fine-tune Qwen on Yuuka dialogues with parameter-efficient LoRA."""

from __future__ import annotations

import argparse
from pathlib import Path
from typing import Dict, List

import torch
from datasets import load_dataset
from peft import LoraConfig, get_peft_model, prepare_model_for_kbit_training
from transformers import (AutoModelForCausalLM, AutoTokenizer, BitsAndBytesConfig,
                          Trainer, TrainingArguments)

DEFAULT_MODEL_NAME = "Qwen/Qwen2.5-7B-Instruct"
DEFAULT_DATA_DIR = Path("data/processed")
DEFAULT_OUTPUT_DIR = Path("models/qwen_yuuka_lora")
MAX_SEQ_LEN = 1024


def get_tokenizer(model_name: str):
    tokenizer = AutoTokenizer.from_pretrained(model_name, trust_remote_code=True)
    if tokenizer.pad_token is None:
        tokenizer.pad_token = tokenizer.eos_token
    tokenizer.padding_side = "right"
    return tokenizer


def load_model(model_name: str):
    quant_config = BitsAndBytesConfig(
        load_in_4bit=True,
        bnb_4bit_quant_type="nf4",
        bnb_4bit_use_double_quant=True,
        bnb_4bit_compute_dtype=torch.float16,
    )

    model = AutoModelForCausalLM.from_pretrained(
        model_name,
        quantization_config=quant_config,
        device_map="auto",
        trust_remote_code=True,
    )
    model = prepare_model_for_kbit_training(model)
    model.config.use_cache = False

    lora_config = LoraConfig(
        r=16,
        lora_alpha=32,
        lora_dropout=0.05,
        target_modules=["q_proj", "k_proj", "v_proj", "o_proj"],
        task_type="CAUSAL_LM",
    )
    model = get_peft_model(model, lora_config)
    model.print_trainable_parameters()
    return model


class PromptCompletionCollator:
    def __init__(self, tokenizer, max_seq_len: int):
        self.tokenizer = tokenizer
        self.max_seq_len = max_seq_len

    def __call__(self, batch: List[Dict[str, str]]) -> Dict[str, torch.Tensor]:
        input_ids = []
        labels = []
        attention_masks = []

        for sample in batch:
            prompt = sample["prompt"]
            completion = sample["completion"].replace("<eos>", self.tokenizer.eos_token)

            prompt_ids = self.tokenizer.encode(prompt, add_special_tokens=False)
            completion_ids = self.tokenizer.encode(
                completion, add_special_tokens=False
            )

            combined_ids = prompt_ids + completion_ids
            combined_labels = [-100] * len(prompt_ids) + completion_ids

            if len(combined_ids) > self.max_seq_len:
                combined_ids = combined_ids[: self.max_seq_len]
                combined_labels = combined_labels[: self.max_seq_len]

            attention_mask = [1] * len(combined_ids)

            # Pad to max length for the batch.
            input_ids.append(combined_ids)
            labels.append(combined_labels)
            attention_masks.append(attention_mask)

        batch_max = max(len(ids) for ids in input_ids)
        padded_input_ids = []
        padded_labels = []
        padded_masks = []

        for ids, lbls, mask in zip(input_ids, labels, attention_masks):
            pad_len = batch_max - len(ids)
            padded_input_ids.append(ids + [self.tokenizer.pad_token_id] * pad_len)
            padded_masks.append(mask + [0] * pad_len)
            padded_labels.append(lbls + [-100] * pad_len)

        return {
            "input_ids": torch.tensor(padded_input_ids, dtype=torch.long),
            "attention_mask": torch.tensor(padded_masks, dtype=torch.long),
            "labels": torch.tensor(padded_labels, dtype=torch.long),
        }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Train a LoRA adapter for Yuuka")
    parser.add_argument(
        "--model-name",
        type=str,
        default=DEFAULT_MODEL_NAME,
        help="Base model to fine-tune.",
    )
    parser.add_argument(
        "--data-dir",
        type=Path,
        default=DEFAULT_DATA_DIR,
        help="Directory with train.jsonl and eval.jsonl produced by preprocess_data.py.",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=DEFAULT_OUTPUT_DIR,
        help="Where to store the trained LoRA adapter.",
    )
    parser.add_argument(
        "--num-epochs", type=float, default=3, help="Number of training epochs."
    )
    parser.add_argument(
        "--eval-steps",
        type=int,
        default=100,
        help="Run evaluation every N steps (0 disables step evaluations).",
    )
    parser.add_argument(
        "--logging-steps", type=int, default=10, help="Logging interval in steps."
    )
    return parser.parse_args()


def load_data(data_dir: Path):
    data_files = {"train": data_dir / "train.jsonl"}
    eval_path = data_dir / "eval.jsonl"
    if eval_path.exists():
        data_files["validation"] = eval_path
    dataset = load_dataset("json", data_files={k: str(v) for k, v in data_files.items()})
    return dataset


def main() -> None:
    args = parse_args()

    tokenizer = get_tokenizer(args.model_name)
    model = load_model(args.model_name)

    dataset = load_data(args.data_dir)

    collator = PromptCompletionCollator(tokenizer, max_seq_len=MAX_SEQ_LEN)

    has_validation = "validation" in dataset
    if has_validation:
        evaluation_strategy = "steps" if args.eval_steps > 0 else "epoch"
    else:
        evaluation_strategy = "no"
    eval_steps = args.eval_steps if has_validation and args.eval_steps > 0 else None

    training_args = TrainingArguments(
        output_dir=str(args.output_dir),
        per_device_train_batch_size=1,
        gradient_accumulation_steps=8,
        gradient_checkpointing=True,
        learning_rate=1e-4,
        lr_scheduler_type="cosine",
        warmup_ratio=0.03,
        optim="paged_adamw_8bit",
        num_train_epochs=args.num_epochs,
        logging_steps=args.logging_steps,
        evaluation_strategy=evaluation_strategy,
        eval_steps=eval_steps,
        save_strategy="epoch",
        save_total_limit=2,
        bf16=torch.cuda.is_available(),
        dataloader_num_workers=2,
        report_to=[],
    )

    trainer = Trainer(
        model=model,
        tokenizer=tokenizer,
        args=training_args,
        train_dataset=dataset["train"],
        eval_dataset=dataset.get("validation"),
        data_collator=collator,
    )

    trainer.train()

    args.output_dir.mkdir(parents=True, exist_ok=True)
    trainer.model.save_pretrained(args.output_dir)
    tokenizer.save_pretrained(args.output_dir)


if __name__ == "__main__":
    main()
