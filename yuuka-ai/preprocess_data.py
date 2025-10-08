"""Preprocess Yuuka dialogue data for supervised fine-tuning.

This script converts dialogue logs stored in ``yuuka_dialogues_zh.jsonl``
into prompt/completion pairs that follow the requirements discussed in the
project brief:

* Treat any message whose ``role`` is ``"优香"`` as the target response.
* Treat every other message as context.
* Build prompts that contain the full context with ``[角色]`` prefixes,
  followed by the ``优香`` prefix ready for completion.
* Store completions that contain the current ``优香`` reply terminated by
  ``<eos>``. The training script will later replace the literal ``<eos>``
  marker with the tokenizer specific end-of-sequence token.
* Only the completion should contribute to the loss – the training script
  masks the prompt portion with ``-100`` during collation.

Usage
-----
python preprocess_data.py \
    --input data/yuuka_dialogues_zh.jsonl \
    --output-dir data/processed \
    --eval-ratio 0.1
"""

from __future__ import annotations

import argparse
import json
import random
from pathlib import Path
from typing import Dict, Iterable, List, Sequence

Sample = Dict[str, str]
Message = Dict[str, str]


def load_conversations(path: Path) -> Iterable[Sequence[Message]]:
    """Yield message sequences from a JSONL dialogue log."""
    with path.open("r", encoding="utf-8") as f:
        for line_number, line in enumerate(f, start=1):
            line = line.strip()
            if not line:
                continue
            try:
                obj = json.loads(line)
            except json.JSONDecodeError as exc:  # pragma: no cover - defensive
                raise ValueError(
                    f"Invalid JSON on line {line_number} of {path}: {exc}"
                ) from exc
            messages = obj.get("messages")
            if not isinstance(messages, list):
                raise ValueError(
                    f"Expected a list of messages on line {line_number} of {path}."
                )
            yield messages


def build_samples(messages: Sequence[Message]) -> List[Sample]:
    """Convert a sequence of chat messages into training samples."""
    context_lines: List[str] = []
    samples: List[Sample] = []

    for message in messages:
        role = message.get("role")
        content = (message.get("content") or "").strip()
        if not role:
            # Skip malformed messages but keep context intact.
            continue

        prefixed_line = f"[{role}] {content}\n"
        if role == "优香":
            prompt = "".join(context_lines) + "[优香] "
            completion = f"{content}<eos>"
            samples.append({"prompt": prompt, "completion": completion})
        context_lines.append(prefixed_line)

    return samples


def split_samples(samples: List[Sample], eval_ratio: float, seed: int = 42) -> Dict[str, List[Sample]]:
    """Shuffle and split samples into train/eval subsets."""
    if not samples:
        raise ValueError("No samples generated from the provided conversations.")

    rng = random.Random(seed)
    rng.shuffle(samples)

    eval_size = int(len(samples) * eval_ratio)
    eval_size = max(1, eval_size) if 0 < eval_ratio < 1.0 else 0
    eval_samples = samples[:eval_size]
    train_samples = samples[eval_size:]

    if not train_samples:
        raise ValueError("Evaluation split is too large; no training samples left.")

    return {"train": train_samples, "eval": eval_samples}


def dump_samples(samples: Iterable[Sample], path: Path) -> None:
    """Write samples to ``path`` in JSONL format."""
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as f:
        for sample in samples:
            json.dump(sample, f, ensure_ascii=False)
            f.write("\n")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Preprocess Yuuka dialogue data")
    parser.add_argument(
        "--input",
        type=Path,
        default=Path("data/yuuka_dialogues_zh.jsonl"),
        help="Path to the raw Yuuka dialogue JSONL file.",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("data/processed"),
        help="Directory where processed JSONL files will be written.",
    )
    parser.add_argument(
        "--eval-ratio",
        type=float,
        default=0.1,
        help="Fraction of samples to reserve for evaluation (0–1).",
    )
    parser.add_argument(
        "--seed", type=int, default=42, help="Random seed for data shuffling."
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    conversations = load_conversations(args.input)

    all_samples: List[Sample] = []
    for messages in conversations:
        all_samples.extend(build_samples(messages))

    splits = split_samples(all_samples, args.eval_ratio, seed=args.seed)

    train_path = args.output_dir / "train.jsonl"
    eval_path = args.output_dir / "eval.jsonl"

    dump_samples(splits["train"], train_path)
    if splits["eval"]:
        dump_samples(splits["eval"], eval_path)

    print(f"Wrote {len(splits['train'])} training samples to {train_path}")
    if splits["eval"]:
        print(f"Wrote {len(splits['eval'])} evaluation samples to {eval_path}")


if __name__ == "__main__":
    main()
