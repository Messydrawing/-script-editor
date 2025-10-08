#!/usr/bin/env python
"""命令行聊天脚本，使用微调后的优香模型。"""

import argparse
from typing import List, Dict

import torch
from transformers import AutoModelForCausalLM, AutoTokenizer
from peft import PeftModel


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Chat with fine-tuned Yuuka model")
    parser.add_argument("--base-model", default="Qwen/Qwen1.5-0.5B", help="基础模型 ID")
    parser.add_argument("--peft-model", default="models/qwen_yuuka_lora", help="LoRA 权重路径")
    parser.add_argument("--max-new-tokens", type=int, default=256, help="生成的最大 token 数")
    parser.add_argument("--temperature", type=float, default=0.8, help="采样温度")
    parser.add_argument("--top-p", type=float, default=0.9, help="nucleus sampling 的 top-p")
    parser.add_argument("--system-prompt", default="你是游戏《碧蓝档案》中角色优香，请以她温柔而直接的语气回答用户的问题。",
                        help="系统提示词")
    return parser.parse_args()


def load_model(base_model: str, peft_model_path: str):
    tokenizer = AutoTokenizer.from_pretrained(base_model)
    model = AutoModelForCausalLM.from_pretrained(base_model, torch_dtype=torch.float16, device_map="auto")
    model = PeftModel.from_pretrained(model, peft_model_path)
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    model.to(device)
    model.eval()
    return tokenizer, model, device


def generate_reply(tokenizer, model, device, history: List[Dict[str, str]], max_new_tokens: int, temperature: float, top_p: float) -> str:
    conversation = tokenizer.apply_chat_template(history, tokenize=False, add_generation_prompt=True)
    inputs = tokenizer([conversation], return_tensors="pt").to(device)
    with torch.inference_mode():
        outputs = model.generate(
            **inputs,
            max_new_tokens=max_new_tokens,
            do_sample=True,
            top_p=top_p,
            temperature=temperature,
        )
    generated_ids = outputs[0][inputs["input_ids"].shape[-1]:]
    reply = tokenizer.decode(generated_ids, skip_special_tokens=True).strip()
    return reply


def main() -> None:
    args = parse_args()
    tokenizer, model, device = load_model(args.base_model, args.peft_model)

    history: List[Dict[str, str]] = []
    if args.system_prompt:
        history.append({"role": "system", "content": args.system_prompt})

    print("开始与优香的对话（输入 exit 结束）：")
    while True:
        user_input = input("用户: ").strip()
        if user_input.lower() in {"exit", "quit"}:
            print("结束对话，再见！")
            break
        if not user_input:
            continue
        history.append({"role": "user", "content": user_input})
        reply = generate_reply(tokenizer, model, device, history, args.max_new_tokens, args.temperature, args.top_p)
        print("优香:", reply)
        history.append({"role": "assistant", "content": reply})
        # 可选：限制历史长度
        if len(history) > 12:
            history = [history[0]] + history[-11:]


if __name__ == "__main__":
    main()
