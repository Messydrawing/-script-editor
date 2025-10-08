# Yuuka LoRA Fine-tuning Project

该项目展示了如何在 8GB 显存环境下，利用 LoRA 对 Qwen 模型进行优香角色的微调，并提供完整的数据处理、训练与推理脚本。

## 项目结构

```
yuuka-ai/
├── data/
│   ├── yuuka_dialogues_zh.jsonl   # 原始优香对话数据（需自行放置）
│   ├── train.jsonl                # 预处理后训练集（运行预处理脚本生成）
│   └── val.jsonl                  # 预处理后验证集（运行预处理脚本生成）
├── scripts/
│   ├── preprocess_data.py         # 数据预处理脚本
│   ├── train_lora.py              # LoRA 微调脚本
│   └── chat_cli.py                # 推理聊天 CLI 脚本
├── models/
│   └── qwen_yuuka_lora/           # 训练得到的 LoRA 权重和 tokenizer
├── requirements.txt               # Python 依赖列表
└── README.md                      # 项目说明（本文档）
```

> **提示**：`data/yuuka_dialogues_zh.jsonl` 为原始数据文件，请在运行脚本前将其放置到该目录下。该文件体积可能较大，通常不纳入版本控制。

## 快速开始

### 1. 安装依赖

```bash
pip install -r requirements.txt
```

如需在较低版本 CUDA 或 CPU 上运行，请参考各依赖库的官方安装说明进行适配。

### 2. 数据预处理

```bash
python scripts/preprocess_data.py \
  --input data/yuuka_dialogues_zh.jsonl \
  --train-output data/train.jsonl \
  --val-output data/val.jsonl \
  --val-ratio 0.1
```

脚本将原始对话拆分为 prompt-completion 样本，并按比例划分训练集与验证集。

### 3. LoRA 微调

```bash
python scripts/train_lora.py \
  --model-id Qwen/Qwen1.5-0.5B \
  --train-file data/train.jsonl \
  --val-file data/val.jsonl \
  --output-dir models/qwen_yuuka_lora
```

该脚本默认使用 4-bit 量化与 LoRA 适配，以降低显存占用。可根据需要调整超参数。

### 4. 命令行对话

在完成微调后，可使用 CLI 与模型对话：

```bash
python scripts/chat_cli.py \
  --base-model Qwen/Qwen1.5-0.5B \
  --peft-model models/qwen_yuuka_lora
```

输入 `exit` 或 `quit` 结束对话。

## 训练产物与日志

建议将训练中产生的 checkpoints、日志等保存到单独目录（例如 `outputs/` 或 `runs/`），并在 `.gitignore` 中忽略，以保持仓库整洁。

## 许可证

根据原始数据与基础模型的授权条款，选择合适的许可证并遵循相关使用限制。
