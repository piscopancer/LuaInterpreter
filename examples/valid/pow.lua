-- Test power operator (^) in Lua 5.5
print("=== POWER OPERATOR (^) TESTS ===")

-- Integer powers
print("2^3 =", 2^3)        -- 8
print("3^2 =", 3^2)        -- 9
print("5^0 =", 5^0)        -- 1
print("0^5 =", 0^5)        -- 0

-- Floating point powers
print("\nFloating point:")
print("2^3.5 =", 2^3.5)    -- ~11.3137
print("4^0.5 =", 4^0.5)    -- 2 (square root)
print("9^0.5 =", 9^0.5)    -- 3

-- Negative bases
print("\nNegative bases:")
print("(-2)^3 =", (-2)^3)  -- -8
print("(-2)^2 =", (-2)^2)  -- 4
print("(-2)^0.5 =", (-2)^0.5) -- NaN (or error)

-- Negative exponents
print("\nNegative exponents:")
print("2^-3 =", 2^-3)      -- 0.125
print("10^-2 =", 10^-2)    -- 0.01

-- Very large/small numbers
print("\nLarge numbers:")
print("10^6 =", 10^6)      -- 1000000
print("10^-6 =", 10^-6)    -- 0.000001

print("\nAll power operator tests completed!")