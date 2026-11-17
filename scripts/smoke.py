"""Check a running local service and require a correct arithmetic answer."""
import argparse
import json
import urllib.request

p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--port", type=int, default=8081)
a = p.parse_args()
base = f"http://127.0.0.1:{a.port}"
health = json.load(urllib.request.urlopen(base + "/health", timeout=15))
if health.get("status") != "ok" or not health.get("loaded") or health.get("max_context") != 262144:
    raise SystemExit("Service not loaded with the expected 256K capacity: " + json.dumps(health))
body = {"model": "qwen3.8-flash-next-gsq-rco-iq3_s-mi50", "messages": [{"role": "user", "content": "17加25等于多少？只回答数字。"}],
        "max_tokens": 64, "temperature": 0, "chat_template_kwargs": {"enable_thinking": False}}
req = urllib.request.Request(base + "/v1/chat/completions", data=json.dumps(body).encode(), headers={"Content-Type": "application/json"})
result = json.load(urllib.request.urlopen(req, timeout=180))
answer = result["choices"][0]["message"].get("content", "")
if "42" not in answer:
    raise SystemExit("Unexpected answer: " + repr(answer))
print("PASS: loaded, advertised 262144 context, arithmetic answer 42. Full-length context and quality were not retested.")
