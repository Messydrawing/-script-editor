# -script-editor

## Yuuka LoRA Training Utilities

The `yuuka-ai` directory contains helper scripts for preparing dialogue data and
training a LoRA adapter on top of Qwen/Qwen2.5-7B-Instruct within an 8 GB GPU
budget.

1. **Preprocess** the raw `yuuka_dialogues_zh.jsonl` file:

   ```bash
   python yuuka-ai/preprocess_data.py \
       --input data/yuuka_dialogues_zh.jsonl \
       --output-dir data/processed \
       --eval-ratio 0.1
   ```

   This converts each dialogue turn into prompt/completion pairs where only the
   `优香` responses contribute to the training loss.

2. **Train** the LoRA adapter with 4-bit quantization:

   ```bash
   python yuuka-ai/train_lora.py \
       --data-dir data/processed \
       --output-dir models/qwen_yuuka_lora
   ```

   The script enables gradient checkpointing, paged AdamW 8-bit optimizers, and
   LoRA adapters on the attention projection layers to remain within the target
   memory footprint. Only the adapter weights are saved in the output directory.
