-- Test bitwise operators in Lua 5.5
print("=== BITWISE OPERATORS TESTS ===")

-- Bitwise AND (&)
print("AND (&):")
print("5 & 3 =", 5 & 3)        -- 1 (0101 & 0011 = 0001)
print("7 & 12 =", 7 & 12)      -- 4 (0111 & 1100 = 0100)
print("0xFF & 0x0F =", 255 & 255)  -- 15

-- Bitwise OR (|)
print("\nOR (|):")
print("5 | 3 =", 5 | 3)        -- 7 (0101 | 0011 = 0111)
print("8 | 4 =", 8 | 4)        -- 12

-- Bitwise XOR (..)
print("\nXOR (~/..):")
print("5 ~ 3 =", 5 ~ 3)        -- 6 (0101 ^ 0011 = 0110)
print("12 ~ 7 =", 12 ~ 7)      -- 11

-- Bitwise NOT (~)
print("\nNOT (~):")
print("~5 =", ~5)              -- -6
print("~0 =", ~0)              -- -1

-- Left shift (<<)
print("\nLeft Shift (<<):")
print("5 << 1 =", 5 << 1)      -- 10
print("3 << 3 =", 3 << 3)      -- 24

-- Right shift (>>)
print("\nRight Shift (>>):")
print("10 >> 1 =", 10 >> 1)    -- 5
print("24 >> 3 =", 24 >> 3)    -- 3

-- Edge cases
print("\nEdge cases:")
print("0 >> 100 =", 0 >> 100)  -- 0
print("1 << 63 =", 1 << 63)    -- Large number
print("~0 >> 1 =", ~0 >> 1)    -- Large positive number

print("\nAll bitwise tests completed!")