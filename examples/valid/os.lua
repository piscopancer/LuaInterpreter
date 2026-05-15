-- Complete OS Library Tests for Lua 5.5
-- Each os function has its own test function

print("=" .. string.rep("=", 70))
print("OS LIBRARY COMPREHENSIVE TESTS")
print("=" .. string.rep("=", 70))

-- Helper function for section headers
global function section(title)
    print("\n" .. string.rep("=", 70))
    print(">>> " .. title)
    print(string.rep("=", 70))
end

-- Helper function for file operations
global function file_exists(name)
    local f = io.open(name, "r")
    if f then f:close(); return true end
    return false
end

global function create_test_file(name, content)
    local f = io.open(name, "w")
    if f then
        if (content) then f:write(content)
        else f:write("Test content for " .. name)
        end
        f:close()
        return true
    end
    return false
end

-- ====================================================================
-- os.clock() tests
-- ====================================================================
function test_os_clock()
    section("os.clock() - CPU Time Tests")
    
    -- Test 1: Basic functionality
    local t1 = os.clock()
    print( type(t1) == "Number",  "Returns number type")
    print("    Value:", t1)
    
    -- Test 2: Time increases with computation
    local sum = 0
    for i = 1, 100000 do sum = sum + i end
    local t2 = os.clock()
    print(t2 > t1, "Time increases after computation (" .. (t2-t1) .. " seconds)")
    
    -- Test 3: Resolution test
    local res = os.clock()
    while os.clock() == res do end
    local resolution = os.clock() - res
    print(resolution > 0, "Clock has measurable resolution (~" .. resolution .. " seconds)")
    
    -- Test 4: Multiple measurements
    local measurements = {}
    for i = 1, 3 do
        local start = os.clock()
        for j = 1, 100000 do local _ = j * j end
        measurements[i] = os.clock() - start
    end
    print(measurements[1] > 0 and measurements[2] > 0, 
                "Consistent measurements: " .. string.format("%.4f, %.4f, %.4f", 
                measurements[1], measurements[2], measurements[3]))
    
    -- Test 5: Monotonic behavior
    local prev = os.clock()
    local monotonic = true
    for i = 1, 1000 do
        local curr = os.clock()
        if curr < prev then
            monotonic = false
            break
        end
        prev = curr
    end
    print(monotonic, "Clock is monotonic (never decreases)")
end

-- ====================================================================
-- os.date() tests
-- ====================================================================
function test_os_date()
    section("os.date() - Date Formatting Tests")
    
    -- Test 1: No arguments
    local current = os.date()
    print(type(current) == "String", "Returns string without arguments")
    print("    Current date:", current)
    
    -- Test 2: Format string
    local formatted = os.date("%Y-%m-%d %H:%M:%S")
    print(#formatted > 0, "Format string works: " .. formatted)
    
    -- Test 3: All format specifiers
    local specifiers = {
        year = os.date("%Y"),
        month = os.date("%m"),
        day = os.date("%d"),
        hour = os.date("%H"),
        minute = os.date("%M"),
        second = os.date("%S"),
        weekday = os.date("%w"),
        month_name = os.date("%B"),
        day_name = os.date("%A")
    }
    print("    Date components:")
    for k, v in pairs(specifiers) do
        print("      " .. k .. ": " .. v)
    end
    print(specifiers.year and specifiers.month, "All format specifiers work")
    
    -- Test 4: With timestamp
    local timestamp = os.time( {year=2000, month=1, day=1} )
    local date_str = os.date("%Y-%m-%d", timestamp)
    print(date_str == "2000-01-01", "Timestamp parameter works: " .. date_str)
    
    -- Test 5: Table return
    local dt = os.date("*t")
    print(type(dt) == "Table", "Returns table with '*t'")
    local required_fields = {"year", "month", "day", "hour", "min", "sec", "wday", "yday"}
    for _, field in ipairs(required_fields) do
        print(dt[field] ~= nil, "Table contains '" .. field .. "' field")
    end
    print(string.format("    Date: %d-%02d-%02d %02d:%02d:%02d", 
          dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec))
    
    -- Test 6: UTC time
    local utc = os.date("!%Y-%m-%d %H:%M:%S")
    local local_time = os.date("%Y-%m-%d %H:%M:%S")
    print(utc ~= local_time, "UTC time differs from local time")
    print("    UTC:", utc)
    print("    Local:", local_time)
    
    -- Test 7: Edge cases
    print(os.date("") ~= nil, "Empty format string works")
    print(os.date("%%") == "%", "Percent literal works")
end

-- ====================================================================
-- os.difftime() tests
-- ====================================================================
function test_os_difftime()
    section("os.difftime() - Time Difference Tests")
    
    -- Test 1: Positive difference
    local t1 = os.time()
    local t2 = t1 + 5
    local diff = os.difftime(t2, t1)
    print(math.abs(diff - 5) < 0.01, "5 second difference: " .. diff)
    
    -- Test 2: Negative difference
    diff = os.difftime(t1, t2)
    print(diff < 0, "Negative difference works: " .. diff)
    
    -- Test 3: Zero difference
    diff = os.difftime(t1, t1)
    print(diff == 0, "Zero difference: " .. diff)
    
    -- Test 4: Day difference
    local day1 = os.time({year=2024, month=1, day=1})
    local day2 = os.time({year=2024, month=1, day=2})
    diff = os.difftime(day2, day1) / 86400
    print(math.abs(diff - 1) < 0.01, "One day difference: " .. diff .. " days")
    
    -- Test 5: Year difference (including leap year)
    local y2023 = os.time({year=2023, month=1, day=1})
    local y2024 = os.time({year=2024, month=1, day=1})
    local days_in_2023 = os.difftime(y2024, y2023) / 86400
    print(math.abs(days_in_2023 - 365) < 1, "2023 has ~365 days: " .. days_in_2023)
    
    local y2024_start = os.time({year=2024, month=1, day=1})
    local y2025 = os.time({year=2025, month=1, day=1})
    local days_in_2024 = os.difftime(y2025, y2024_start) / 86400
    print(math.abs(days_in_2024 - 366) < 1, "2024 (leap) has ~366 days: " .. days_in_2024)
    
    -- Test 6: Large difference
    local epoch = os.time({year=1970, month=1, day=1})
    local now = os.time()
    local seconds_since_epoch = os.difftime(now, epoch)
    print(seconds_since_epoch > 0, "Seconds since epoch: " .. seconds_since_epoch)
end

-- ====================================================================
-- os.execute() tests
-- ====================================================================
function test_os_execute()
    section("os.execute() - System Command Tests")
    
    -- Test 1: Simple command
    local success, output, code = os.execute("echo Hello from Lua")
    print(success == true or success == nil, "Command executed")
    print("    Exit code:", code)
    if output then
        print("    Output preview:", output:sub(1, 80))
    end
    
    -- Test 2: Command with error
    success, output, code = os.execute("nonexistent_command_xyz_123")
    print(success == false, "Failed command returns false")
    print("    Error code:", code)
    
    -- Test 3: Directory listing (platform independent)
    local cmd = "ls -1"
    success, output, code = os.execute(cmd)
    print(success == true or success == nil, "Directory listing works")
    
    -- Test 5: Command with quotes
    success, output, code = os.execute("echo \"Quoted argument test\"")
    print(success == true or success == nil, "Command with quotes works")
end

-- ====================================================================
-- os.getenv() tests
-- ====================================================================
function test_os_getenv()
    section("os.getenv() - Environment Variable Tests")
    
    -- Test 1: Get existing variable
    local path = os.getenv("PATH")
    print(path ~= nil and type(path) == "String", "PATH variable exists")
    if path then
        print("    PATH length:", #path)
        print("    PATH:", path)
    end
    
    -- Test 2: Get non-existent variable
    local missing = os.getenv("NONEXISTENT_VARIABLE_XYZ")
    print(missing == nil, "Non-existent variable returns nil")
    
    -- Test 3: Platform-specific variables
    local home = os.getenv("HOME")
    if not home then
        home = os.getenv("USERPROFILE")
    end
    print(home ~= nil, "Home directory variable exists: " .. (home or "none"))
    
    local user = os.getenv("USER")
    if not user then
        user = os.getenv("USERNAME")
    end
    print(user ~= nil, "Username variable exists: " .. (user or "none"))
    
    -- Test 4: Case sensitivity
    local path_upper = os.getenv("PATH")
    local path_lower = os.getenv("path")
    if path_upper and path_lower then
        if path_upper == path_lower then
            print(true, "Environment is case-insensitive")
        else
            print(true, "Environment is case-sensitive")
        end
    else
        print(true, "Case sensitivity test skipped")
    end
    
    -- Test 5: Common environment variables
    local common_vars = {"TEMP", "TMP", "LANG", "SHELL", "OS", "COMPUTERNAME"}
    local found = 0
    for _, var in ipairs(common_vars) do
        if os.getenv(var) then
            found = found + 1
        end
    end
    print(found > 0, string.format("Found %d/%d common environment variables", found, #common_vars))
    
    -- Test 6: Performance
    local start = os.clock()
    for i = 1, 10000 do
        os.getenv("PATH")
    end
    local elapsed = os.clock() - start
    print(elapsed < 1, string.format("10,000 calls in %.4f seconds", elapsed))
end

-- ====================================================================
-- os.remove() tests
-- ====================================================================
function test_os_remove()
    section("os.remove() - File Removal Tests")
    
    -- Setup: Create test files
    local test_file = "test_remove_temp.txt"
    local test_file2 = "test_remove_temp2.txt"
    create_test_file(test_file, "Test content")
    create_test_file(test_file2, "More content")
    print(file_exists(test_file), "Test file created")
    
    -- Test 1: Remove existing file
    local result = os.remove(test_file)
    print(result == true, "Remove existing file: " .. result)
    print(not file_exists(test_file), "File no longer exists")
    
    -- Test 2: Remove non-existent file
    local success, err, code = os.remove("nonexistent_file_xyz.txt")
    print(success == false, "Remove non-existent file returns false")
    if err then
        print("    Error message:", err)
        print("    Error code:", code)
    end
    
    -- Test 3: Remove file with spaces
    local spaced_file = "test file with spaces.txt"
    create_test_file(spaced_file, "Spaced content")
    result = os.remove(spaced_file)
    print(result == true, "Remove file with spaces")
    
    -- Test 4: Try to remove directory
    local mkdir_cmd
    local rmdir_cmd

    mkdir_cmd = "mkdir temp_remove_dir"
    rmdir_cmd = "rmdir temp_remove_dir"

    os.execute(mkdir_cmd)
    local success_dir, err_dir = os.remove("temp_remove_dir")
    print(success_dir == false, "Cannot remove directory (returns false)")
    if err_dir then
        print("    Error:", err_dir)
    end
    os.execute(rmdir_cmd)
    
    -- Test 5: Remove open file (platform dependent)
    local f = io.open(test_file2, "r")
    if f then
        do
            local result_open, reason = os.remove(test_file2)
            f:close()
            if result_open then
                print(true, "Removed open file (platform dependent)")
            else
                print(true, "Cannot remove open file (expected on some platforms)", reason)
            end    
        end
        os.remove(test_file2) -- Remove after close
    end
    
    -- Test 6: Verify final cleanup
    print(not file_exists(test_file2), "All test files cleaned up")
end

-- ====================================================================
-- os.rename() tests
-- ====================================================================
function test_os_rename()
    section("os.rename() - File Rename Tests")
    
    -- Setup: Create test file
    local old_name = "rename_old.txt"
    local new_name = "rename_new.txt"
    create_test_file(old_name, "Original content")
    print(file_exists(old_name), "Test file created")
    
    -- Test 1: Basic rename
    local result = os.rename(old_name, new_name)
    print(result == true, "Basic rename successful")
    print(not file_exists(old_name), "Old filename no longer exists")
    print(file_exists(new_name), "New filename exists")
    
    -- Test 2: Rename to existing file (overwrite)
    local existing_file = "rename_existing.txt"
    create_test_file(existing_file, "Original content")
    create_test_file(new_name, "New content to overwrite with")
    result = os.rename(new_name, existing_file)
    print(result == true, "Overwrite existing file")
    
    -- Test 3: Rename non-existent file
    local success, err, code = os.rename("nonexistent.txt", "target.txt")
    print(success == false, "Rename non-existent returns false")
    if err then
        print("    Error:", err)
    end
    
    -- Test 4: Rename with spaces
    local spaced_old = "old file name.txt"
    local spaced_new = "new file name.txt"
    create_test_file(spaced_old, "Content")
    result = os.rename(spaced_old, spaced_new)
    print(result == true, "Rename files with spaces")
    os.remove(spaced_new)
    
    -- Test 5: Rename directory (if supported)
    local mkdir_cmd = "mkdir rename_test_dir"
    os.execute(mkdir_cmd)
    local dir_result = os.rename("rename_test_dir", "rename_test_dir_renamed")
    if dir_result then
        print(true, "Directory rename supported")
        os.rename("rename_test_dir_renamed", "rename_test_dir")
    else
        print(true, "Directory rename not supported (expected on some platforms)")
    end
    
    local rmdir_cmd = "rmdir rename_test_dir"
    os.execute(rmdir_cmd)
    
    -- Test 6: Cleanup
    os.remove(existing_file)
    print(not file_exists(existing_file), "Cleanup completed")
end

-- ====================================================================
-- os.setlocale() tests
-- ====================================================================
function test_os_setlocale()
    section("os.setlocale() - Locale Tests")
    
    -- Test 1: Get current locale
    local current = os.setlocale(nil)
    if current then
        print(type(current) == "String", "Current locale: " .. current)
    else
        print(true, "Current locale: C (default)")
    end
    
    -- Test 2: Get by category
    local categories = {"collate", "ctype", "monetary", "numeric", "time"}
    for _, cat in ipairs(categories) do
        local loc = os.setlocale(nil, cat)
        if loc then
            print(string.format("    %s locale: %s", cat, loc))
        end
    end
    print(true, "Category queries work")
    
    -- Test 3: Set C locale
    local result = os.setlocale("C")
    if result then
        print(result == "C", "Set to C locale: " .. (result))
    else
        print(true, "C locale is default")
    end
    
    -- Test 4: Try to set other locales (may fail)
    local test_locales = {"en_US", "en_US.UTF-8", "C.UTF-8"}
    local any_success = false
    for _, loc in ipairs(test_locales) do
        if os.setlocale(loc) then
            any_success = true
            print(true, "Set to " .. loc .. " locale")
            break
        end
    end
    if not any_success then
        print(true, "No additional locales available (using C locale)")
    end
    
    -- Test 5: Set numeric locale
    local old_numeric = os.setlocale(nil, "numeric")
    local numeric_result = os.setlocale("C", "numeric")
    if numeric_result then
        print(numeric_result == "C", "Set numeric category")
        if old_numeric then
            os.setlocale(old_numeric, "numeric")
        end
    else
        print(true, "Cannot set numeric category")
    end
    
    -- Test 6: Invalid locale
    local invalid = os.setlocale("invalid_locale_xyz")
    print(invalid == nil, "Invalid locale returns nil")
    
    -- Test 7: Restore original locale
    if current then
        os.setlocale(current)
        local restored = os.setlocale(nil)
        if restored then
            print(restored == current, "Restored original locale")
        else
            print(true, "Locale restoration attempted")
        end
    end
end

-- ====================================================================
-- os.time() tests
-- ====================================================================
function test_os_time()
    section("os.time() - Time Value Tests")
    
    -- Test 1: No arguments (current time)
    local now = os.time()
    print(type(now) == "Number", "Returns number without arguments")
    print("    Current timestamp:", now)
    
    -- Test 2: With table (full date)
    local dt = {year=2024, month=12, day=25, hour=10, min=30, sec=0}
    local ts = os.time(dt)
    print(ts > 0, "Create timestamp from table: " .. ts)
    
    -- Test 3: Verify timestamp
    local back_to_date = os.date("*t", ts)
    print(back_to_date.year == 2024 and back_to_date.month == 12, 
                "Timestamp converts back correctly")
    
    -- Test 4: Minimal table (only year, month, day)
    local minimal = os.time({year=2024, month=1, day=1})
    print(minimal ~= nil, "Minimal table works")
    
    -- Test 5: With hour/minute/second
    local with_time = os.time({year=2024, month=1, day=1, hour=12, min=0, sec=0})
    print(with_time > minimal, "Time of day affects timestamp")
    
    -- Test 6: Different dates
    local dates = {
        os.time({year=2000, month=1, day=1}),
        os.time({year=2020, month=2, day=29}),  -- Leap day
        os.time({year=2030, month=12, day=31})
    }
    for i, ts in ipairs(dates) do
        print(ts ~= nil, string.format("Date %d timestamp: %d", i, ts))
    end
    
    -- Test 7: Invalid date handling
    local status, err = pcall(function()
        return os.time({year=2024, month=13, day=1})
    end)
    print(not status, "Invalid date (month 13) causes error")
    
    -- Test 8: Time progression
    local t1 = os.time()
    local sum = 0
    for i = 1, 100000 do
        sum = sum + i
    end
    local t2 = os.time()
    print(t2 >= t1, "Time progresses forward: " .. t1 .. " -> " .. t2)
end

-- ====================================================================
-- os.tmpname() tests
-- ====================================================================
function test_os_tmpname()
    section("os.tmpname() - Temporary Filename Tests")
    
    -- Test 1: Basic functionality
    local tmp1 = os.tmpname()
    print(type(tmp1) == "String", "Returns string: " .. tmp1)
    
    -- Test 2: Unique names
    local tmp2 = os.tmpname()
    local tmp3 = os.tmpname()
    print(tmp1 ~= tmp2, "Generates unique names")
    print(tmp2 ~= tmp3, "All names are different")
    
    -- Test 3: Filename format
    print(#tmp1 > 0, "Non-empty filename")
    local has_path_sep = false;

    if ( (tmp1:find("\\")) or (tmp1:find("2")) ) then
        has_path_sep = true
    end
    print(not has_path_sep, "No path separators (filename only)")
    
    -- Test 4: Multiple calls
    local names = {}
    for i = 1, 10 do
        names[i] = os.tmpname()
    end
    local unique = true
    for i = 1, 10 do
        for j = i+1, 10 do
            if names[i] == names[j] then
                unique = false
                break
            end
        end
    end
    print(unique, "All 10 generated names are unique")
    
    -- Test 5: File doesn't exist initially
    local tmp = os.tmpname()
    print(not file_exists(tmp), "Temporary file doesn't exist yet")
    
    -- Test 6: Can create file with this name
    local f = io.open(tmp, "w")
    if f then
        f:write("test")
        f:close()
        print(file_exists(tmp), "Can create file with tmpname")
        os.remove(tmp)
    else
        print(false, "Could not create file with tmpname")
    end
    
    -- Test 7: Different calls produce different names
    local t1 = os.tmpname()
    local t2 = os.tmpname()
    print(t1 ~= t2, "Sequential calls produce different names: " .. t1 .. " vs " .. t2)
    
    -- Test 8: Performance
    local start = os.clock()
    for i = 1, 1000 do
        local _ = os.tmpname()
    end
    local elapsed = os.clock() - start
    print(elapsed < 1, string.format("1000 calls in %.4f seconds", elapsed))
end

-- ====================================================================
-- Run all tests
-- ====================================================================
function run_all_tests()

    print("RUNNING ALL OS LIBRARY TESTS")

    
    local test_functions = {
        test_os_clock,
        test_os_date,
        test_os_difftime,
        test_os_execute,
        test_os_getenv,
        test_os_remove,
        test_os_rename,
        test_os_setlocale,
        test_os_time,
        test_os_tmpname
    }
    
    local total = #test_functions
    local passed = 0
    
    for _, test_func in ipairs(test_functions) do
        local success, err = pcall(test_func)
        if success then
            passed = passed + 1
        else
            print("\nTest failed with error:", err)
        end
    end
    
    print("\n")
    print(string.rep("=", 68))
    print(string.format("RESULTS: %d/%d TESTS PASSED", passed, total))
    print(string.rep("=", 68))
    
    if passed == total then
        print("\nAll OS library tests passed successfully!")
    else
        print(string.format("\n%d test(s) failed. Please check your Lua 5.5 interpreter.", total - passed))
    end
end

-- Run the test suite
run_all_tests()