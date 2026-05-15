-- Test function call syntax with strings
print("=== FUNCTION CALLS WITH STRINGS TESTS ===")

-- Custom require-like function
function load_module(name)
    print("Loading module:", name)
    return { loaded = true, name = name }
end

-- Test different string call syntaxes
print("\n1. Standard function call:")
load_module("test_module")

print("\n2. No parentheses (sugar syntax):")
load_module "test_module"