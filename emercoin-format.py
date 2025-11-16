import sys
import json
import re
def extract_value(text):
    try:
        # Remove non-printable characters (including BOM) and trim whitespace
        cleaned_text = re.sub(r'[^\x20-\x7E]+', '', text).strip()
        # Try to parse JSON after cleaning
        data = json.loads(cleaned_text)
        # Extract and return the 'value' field
        return data.get("value", "No 'value' found")
    except json.JSONDecodeError:
        return f"Invalid JSON input: {cleaned_text}"
if __name__ == "__main__":
    if len(sys.argv) > 1:
        text = sys.argv[1]
    else:
        text = sys.stdin.read()
    print(extract_value(text))
