#!/usr/bin/env python
"""使用 LoRA 对 Qwen 模型进行微调的示例脚本。"""

import argparse
from pathlib import Path

import torch
from datasets import load_dataset
from transformers import (AutoModelForCausalLM, AutoTokenizer, BitsAndBytesConfig,
                          Trainer, TrainingArguments)

from peft import LoraConfig, get_peft_model, prepare_model_for_kbit_training


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Fine-tune Qwen with LoRA")
    parser.add_argument("--model-id", default="Qwen/Qwen1.5-0.5B", help="基础模型 ID")
    parser.add_argument("--train-file", type=str, default="data/train.jsonl", help="训练集 JSONL 路径")
    parser.add_argument("--val-file", type=str, default="data/val.jsonl", help="验证集 JSONL 路径")
    parser.add_argument("--output-dir", type=str, default="models/qwen_yuuka_lora", help="模型保存目录")
    parser.add_argument("--batch-size", type=int, default=1, help="每设备 batch size")
    parser.add_argument("--grad-accum", type=int, default=4, help="梯度累积步数")
    parser.add_argument("--learning-rate", type=float, default=3e-4, help="学习率")
    parser.add_argument("--max-steps", type=int, default=2000, help="训练步数")
    parser.add_argument("--save-steps", type=int, default=500, help="保存 checkpoint 间隔")
    parser.add_argument("--eval-steps", type=int, default=500, help="评估间隔")
    parser.add_argument("--logging-steps", type=int, default=100, help="日志间隔")
    parser.add_argument("--target-modules", nargs="*", default=["q_proj", "k_proj", "v_proj", "o_proj"],
                        help="应用 LoRA 的模块名列表")
    parser.add_argument("--lora-r", type=int, default=16, help="LoRA rank")
    parser.add_argument("--lora-alpha", type=int, default=8, help="LoRA alpha")
    parser.add_argument("--lora-dropout", type=float, default=0.05, help="LoRA dropout")
    parser.add_argument("--bf16", action="store_true", help="使用 bfloat16 训练")
    parser.add_argument("--no-gradient-checkpointing", action="store_true", help="禁用梯度检查点")
    parser.add_argument("--seed", type=int, default=42, help="随机种子")
    return parser.parse_args()


def get_bnb_config() -> BitsAndBytesConfig:
    return BitsAndBytesConfig(
        load_in_4bit=True,
        bnb_4bit_quant_type="nf4",
        bnb_4bit_use_double_quant=True,
        bnb_4bit_compute_dtype=torch.bfloat16,
    )


def data_collator(tokenizer):
    def collate(features):
        input_ids_list, labels_list = [], []
        for ex in features:
            prompt_ids = tokenizer(ex["prompt"], add_special_tokens=False)["input_ids"]
            response_ids = tokenizer(ex["completion"], add_special_tokens=False)["input_ids"] + [tokenizer.eos_token_id]
            input_ids = prompt_ids + response_ids
            labels = [-100] * len(prompt_ids) + response_ids
            input_ids_list.append(input_ids)
            labels_list.append(labels)
        batch = tokenizer.pad({"input_ids": input_ids_list, "labels": labels_list}, padding=True, return_tensors="pt")
        return {"input_ids": batch["input_ids"], "labels": batch["labels"]}

    return collate


def main() -> None:
    args = parse_args()

    train_file = Path(args.train_file)
    val_file = Path(args.val_file)
    if not train_file.exists():
        raise FileNotFoundError(f"未找到训练集: {train_file}")
    if not val_file.exists():
        raise FileNotFoundError(f"未找到验证集: {val_file}")

    tokenizer = AutoTokenizer.from_pretrained(args.model_id)

    bnb_config = get_bnb_config()
    compute_dtype = torch.bfloat16 if args.bf16 else torch.float16

    model = AutoModelForCausalLM.from_pretrained(
        args.model_id,
        quantization_config=bnb_config,
        device_map="auto",
        torch_dtype=compute_dtype,
    )
    model = prepare_model_for_kbit_training(model)

    lora_config = LoraConfig(
        r=args.lora_r,
        lora_alpha=args.lora_alpha,
        target_modules=args.target_modules,
        lora_dropout=args.lora_dropout,
        bias="none",
        task_type="CAUSAL_LM",
    )
    model = get_peft_model(model, lora_config)

    dataset = load_dataset("json", data_files={"train": str(train_file), "validation": str(val_file)})

    training_args = TrainingArguments(
        output_dir=args.output_dir,
        per_device_train_batch_size=args.batch_size,
        gradient_accumulation_steps=args.grad_accum,
        learning_rate=args.learning_rate,
        max_steps=args.max_steps,
        save_steps=args.save_steps,
        eval_steps=args.eval_steps,
        logging_steps=args.logging_steps,
        fp16=not args.bf16,
        bf16=args.bf16,
        gradient_checkpointing=not args.no_gradient_checkpointing,
        report_to="none",
        seed=args.seed,
    )

    trainer = Trainer(
        model=model,
        args=training_args,
        train_dataset=dataset["train"],
        eval_dataset=dataset["validation"],
        tokenizer=tokenizer,
        data_collator=data_collator(tokenizer),
    )

    trainer.train()

    model.save_pretrained(args.output_dir)
    tokenizer.save_pretrained(args.output_dir)


if __name__ == "__main__":
    main()
