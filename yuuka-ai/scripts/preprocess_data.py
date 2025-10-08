#!/usr/bin/env python
"""预处理优香对话数据，生成 prompt-completion 形式的训练与验证集。"""

import argparse
import json
import random
from pathlib import Path
from typing import List, Dict


def load_conversations(path: Path) -> List[Dict]:
    """读取 JSONL 对话数据。"""
    conversations: List[Dict] = []
    with path.open("r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            conversations.append(json.loads(line))
    return conversations


def build_samples(conversations: List[Dict]) -> List[Dict[str, str]]:
    """将对话拆分为 prompt-completion 样本。"""
    samples: List[Dict[str, str]] = []
    for conv in conversations:
        messages = conv.get("messages", [])
        context_lines: List[str] = []
        for message in messages:
            role = message.get("role")
            text = (message.get("content") or "").strip()
            if not text:
                continue
            if role == "assistant":
                prompt = "\n".join(context_lines + ["优香: "])
                samples.append({"prompt": prompt, "completion": text})
                context_lines.append(f"优香: {text}")
            elif role == "user":
                context_lines.append(f"用户: {text}")
            elif role == "system":
                context_lines.append(f"系统: {text}")
            else:
                # 将其它角色统一并入上下文
                context_lines.append(f"{role or '其它'}: {text}")
    return samples


def save_jsonl(path: Path, samples: List[Dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as f:
        for sample in samples:
            f.write(json.dumps(sample, ensure_ascii=False) + "\n")


def split_dataset(samples: List[Dict[str, str]], val_ratio: float, seed: int) -> (List[Dict[str, str]], List[Dict[str, str]]):
    random.Random(seed).shuffle(samples)
    split_idx = int(len(samples) * (1 - val_ratio))
    train_samples = samples[:split_idx]
    val_samples = samples[split_idx:]
    return train_samples, val_samples


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="预处理优香对话数据")
    parser.add_argument("--input", type=Path, default=Path("data/yuuka_dialogues_zh.jsonl"), help="原始 JSONL 数据路径")
    parser.add_argument("--train-output", type=Path, default=Path("data/train.jsonl"), help="训练集输出路径")
    parser.add_argument("--val-output", type=Path, default=Path("data/val.jsonl"), help="验证集输出路径")
    parser.add_argument("--val-ratio", type=float, default=0.1, help="验证集占比，0-1 之间")
    parser.add_argument("--seed", type=int, default=42, help="随机种子")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if not args.input.exists():
        raise FileNotFoundError(f"未找到输入文件: {args.input}")

    conversations = load_conversations(args.input)
    if not conversations:
        raise ValueError("输入文件为空或无有效对话")

    samples = build_samples(conversations)
    if not samples:
        raise ValueError("未能构建任何训练样本，请检查数据格式")

    train_samples, val_samples = split_dataset(samples, args.val_ratio, args.seed)
    if not train_samples:
        raise ValueError("训练集为空，请调整验证集比例")

    save_jsonl(args.train_output, train_samples)
    save_jsonl(args.val_output, val_samples if val_samples else train_samples[:1])
    print(f"生成训练样本 {len(train_samples)} 条，验证样本 {len(val_samples)} 条。")


if __name__ == "__main__":
    main()
