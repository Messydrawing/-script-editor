# -*- coding: utf-8 -*-
"""Fine-tune Qwen on Yuuka dialogues with parameter-efficient LoRA."""

from __future__ import annotations

import argparse
import inspect
from pathlib import Path
from typing import Dict, List

import torch
from datasets import load_dataset
from peft import LoraConfig, get_peft_model, prepare_model_for_kbit_training
from transformers import (
    AutoModelForCausalLM,
    AutoTokenizer,
    BitsAndBytesConfig,
    Trainer,
    TrainingArguments,
)

DEFAULT_MODEL_NAME = "Qwen/Qwen2.5-7B-Instruct"
DEFAULT_DATA_DIR = Path("data/processed")
DEFAULT_OUTPUT_DIR = Path("models/qwen_yuuka_lora")
MAX_SEQ_LEN = 1024


def get_tokenizer(model_name: str):
    tokenizer = AutoTokenizer.from_pretrained(model_name, trust_remote_code=True)
    # 补齐 PAD
    if tokenizer.pad_token is None:
        tokenizer.pad_token = tokenizer.eos_token
    tokenizer.padding_side = "right"
    return tokenizer


def load_model(model_name: str, tokenizer):
    # 4-bit 量化
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
    model.config.use_cache = False  # 训练时关闭 cache

    # 同步 tokenizer 的 PAD/EOS 到 model.config，避免 token 提示
    if hasattr(tokenizer, "pad_token_id") and tokenizer.pad_token_id is not None:
        model.config.pad_token_id = tokenizer.pad_token_id
        try:
            model.generation_config.pad_token_id = tokenizer.pad_token_id
        except Exception:
            pass
    if hasattr(tokenizer, "eos_token_id") and tokenizer.eos_token_id is not None:
        model.config.eos_token_id = tokenizer.eos_token_id

    # LoRA 配置（仅注意力层以省显存）
    lora_config = LoraConfig(
        r=16,
        lora_alpha=32,
        lora_dropout=0.05,
        target_modules=["q_proj", "k_proj", "v_proj", "o_proj"],
        task_type="CAUSAL_LM",
        bias="none",
    )
    model = get_peft_model(model, lora_config)
    model.print_trainable_parameters()
    return model


class PromptCompletionCollator:
    """把 prompt/completion 拼接为 input_ids/labels；对 prompt 打 -100，只训练 completion。"""

    def __init__(self, tokenizer, max_seq_len: int):
        self.tokenizer = tokenizer
        self.max_seq_len = max_seq_len

    def __call__(self, batch: List[Dict[str, str]]) -> Dict[str, torch.Tensor]:
        input_ids, labels, attention_masks = [], [], []

        for sample in batch:
            prompt = sample["prompt"]
            completion = sample["completion"].replace("<eos>", self.tokenizer.eos_token)

            prompt_ids = self.tokenizer.encode(prompt, add_special_tokens=False)
            completion_ids = self.tokenizer.encode(completion, add_special_tokens=False)

            ids = prompt_ids + completion_ids
            lbls = [-100] * len(prompt_ids) + completion_ids

            if len(ids) > self.max_seq_len:
                ids = ids[: self.max_seq_len]
                lbls = lbls[: self.max_seq_len]

            mask = [1] * len(ids)

            input_ids.append(ids)
            labels.append(lbls)
            attention_masks.append(mask)

        batch_max = max(len(x) for x in input_ids)
        pad_id = self.tokenizer.pad_token_id

        def pad(seq, pad_val, length):
            return seq + [pad_val] * (length - len(seq))

        padded_input_ids = [pad(x, pad_id, batch_max) for x in input_ids]
        padded_masks = [pad(x, 0, batch_max) for x in attention_masks]
        padded_labels = [pad(x, -100, batch_max) for x in labels]

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
    parser.add_argument("--num-epochs", type=float, default=3, help="Epochs.")
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
    model = load_model(args.model_name, tokenizer)

    dataset = load_data(args.data_dir)

    collator = PromptCompletionCollator(tokenizer, max_seq_len=MAX_SEQ_LEN)

    # ==== 版本兼容：根据 TrainingArguments 的入参动态拼参 ====
    param_names = set(inspect.signature(TrainingArguments.__init__).parameters)

    def add_if_supported(d: dict, name: str, value):
        if name in param_names and value is not None:
            d[name] = value

    has_validation = "validation" in dataset
    eval_steps = args.eval_steps

    base_kwargs = {}
    if has_validation:
        if "evaluation_strategy" in param_names:
            base_kwargs["evaluation_strategy"] = (
                "steps" if (eval_steps and eval_steps > 0) else "epoch"
            )
            add_if_supported(base_kwargs, "eval_steps", eval_steps if (eval_steps and eval_steps > 0) else None)
        else:
            base_kwargs["do_eval"] = True
            add_if_supported(base_kwargs, "eval_steps", eval_steps if (eval_steps and eval_steps > 0) else None)
    else:
        if "evaluation_strategy" in param_names:
            base_kwargs["evaluation_strategy"] = "no"
        else:
            base_kwargs["do_eval"] = False

    # 保存/日志策略（旧版可能不支持 save_strategy/report_to）
    add_if_supported(base_kwargs, "save_strategy", "epoch")
    add_if_supported(base_kwargs, "save_total_limit", 2)
    add_if_supported(base_kwargs, "logging_steps", args.logging_steps)

    # 训练相关（8GB 安全配置）
    base_kwargs["output_dir"] = str(args.output_dir)
    base_kwargs["per_device_train_batch_size"] = 1
    base_kwargs["gradient_accumulation_steps"] = 8
    add_if_supported(base_kwargs, "gradient_checkpointing", True)
    base_kwargs["learning_rate"] = 1e-4
    add_if_supported(base_kwargs, "lr_scheduler_type", "cosine")
    add_if_supported(base_kwargs, "warmup_ratio", 0.03)
    add_if_supported(base_kwargs, "optim", "paged_adamw_8bit")  # 不支持会被自动忽略
    base_kwargs["num_train_epochs"] = args.num_epochs
    add_if_supported(base_kwargs, "dataloader_num_workers", 2)
    add_if_supported(base_kwargs, "report_to", [])

    # 精度（4060Ti 建议只开 fp16）
    add_if_supported(base_kwargs, "fp16", True)
    add_if_supported(base_kwargs, "bf16", False)

    # 关键：保留原始列，交给自定义 collator 处理
    base_kwargs["remove_unused_columns"] = False

    training_args = TrainingArguments(**base_kwargs)

    trainer = Trainer(
        model=model,
        args=training_args,
        train_dataset=dataset["train"],
        eval_dataset=dataset.get("validation"),
        data_collator=collator,
        tokenizer=tokenizer,  # v5 以后会更名为 processing_class，当前正常可用
    )

    trainer.train()

    args.output_dir.mkdir(parents=True, exist_ok=True)
    trainer.model.save_pretrained(args.output_dir)
    tokenizer.save_pretrained(args.output_dir)


if __name__ == "__main__":
    main()
