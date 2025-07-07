# Test User Checklist - EOPF Zarr GDAL Plugin

Use this checklist to systematically test the plugin and report your results.

## Before You Start

- [ ] Downloaded the latest release from GitHub
- [ ] Extracted the files to a folder
- [ ] Have GDAL installed on your system (check with `gdalinfo --version`)

## Installation Test

### Step 1: Install the Plugin
- [ ] **Windows**: Ran `install-windows.bat` successfully
- [ ] **macOS**: Ran `./install-macos.sh` successfully  
- [ ] **Linux**: Ran `./install-linux.sh` successfully

### Step 2: Verify Installation
- [ ] Plugin appears in formats list: `gdalinfo --formats | grep EOPFZARR`
- [ ] No fatal error messages during installation
- [ ] ⚠️ **Windows users**: Error 126 is expected and harmless

**Result**: ✅ Pass / ❌ Fail

**Notes**: _Write any error messages or issues here_

---

## Basic Functionality Test

### Step 3: Test Plugin Registration
```bash
gdalinfo --formats | grep EOPFZARR
```

**Expected output**: 
```
EOPFZARR -raster- (rovs): EOPF Zarr Wrapper Driver
```

- [ ] Plugin shows up in the list
- [ ] No error messages when running the command

**Result**: ✅ Pass / ❌ Fail

### Step 4: Test with Sample Data (if available)
If you have access to EOPF Zarr datasets:

```bash
gdalinfo your-dataset.zarr
```

- [ ] Dataset opens without fatal errors
- [ ] Shows dataset information (size, bands, etc.)
- [ ] Driver is identified as EOPFZARR

**Result**: ✅ Pass / ❌ Fail / ⏭️ Skip (no test data)

**Notes**: _Describe what information was displayed_

---

## Application Integration Test

### Step 5: Test in QGIS (if available)
- [ ] QGIS opens without errors
- [ ] Can access "Add Raster Layer" dialog
- [ ] Can browse to and select .zarr files/directories
- [ ] Dataset loads successfully in QGIS

**Result**: ✅ Pass / ❌ Fail / ⏭️ Skip (no QGIS)

### Step 6: Test with Python (if available)
```python
from osgeo import gdal

# Check if plugin is available
driver = gdal.GetDriverByName('EOPFZARR')
print(f"Driver found: {driver is not None}")

# Try to open a dataset (if you have one)
# dataset = gdal.OpenEx("your-dataset.zarr", gdal.OF_MULTIDIM_RASTER)
```

- [ ] Python can import GDAL
- [ ] EOPFZARR driver is found
- [ ] Can open datasets (if test data available)

**Result**: ✅ Pass / ❌ Fail / ⏭️ Skip (no Python/GDAL)

---

## Platform-Specific Tests

### macOS Users
- [ ] Correct architecture detected automatically
- [ ] No architecture mismatch errors
- [ ] Universal binary works (test with `./install-macos.sh universal`)

**Your Mac**: 
- [ ] Intel (x86_64)
- [ ] Apple Silicon (M1/M2/M3 - arm64)

### Windows Users
- [ ] DLL loads (with or without Error 126)
- [ ] Plugin still functions despite any error messages
- [ ] GDAL_DRIVER_PATH is set correctly

### Linux Users
- [ ] Shared library loads correctly
- [ ] No missing dependency errors
- [ ] Plugin installs to correct location

---

## Performance & Stability Test

### Step 7: Stress Test (optional)
If you have large datasets or want to test thoroughly:

- [ ] Plugin handles large files without crashing
- [ ] Memory usage seems reasonable
- [ ] No memory leaks or crashes during extended use

**Result**: ✅ Pass / ❌ Fail / ⏭️ Skip

---

## Issues Encountered

### Error Messages
_Copy any error messages you encountered:_

```
[Paste error messages here]
```

### System Information
Please provide this information when reporting issues:

**Operating System**: _Windows 10/11, macOS version, Linux distribution_

**Architecture**: _x86_64, arm64, etc._

**GDAL Version**: 
```bash
gdalinfo --version
```
_Result:_

**Plugin Status**:
```bash
gdalinfo --formats | grep EOPFZARR
```
_Result:_

### What Worked
- [ ] Installation
- [ ] Plugin registration
- [ ] Dataset opening
- [ ] QGIS integration
- [ ] Python integration
- [ ] No crashes or major issues

### What Didn't Work
_Describe any problems:_

### Unexpected Behavior
_Anything that seemed wrong or strange:_

---

## Overall Assessment

### Should this plugin be considered "working" on your system?

- [ ] **Yes** - Everything works as expected
- [ ] **Yes with caveats** - Works but has harmless error messages
- [ ] **Partially** - Some features work, others don't
- [ ] **No** - Major issues prevent basic functionality

### Recommendation for Other Users
- [ ] **Recommend** - Ready for other test users
- [ ] **Recommend with notes** - Works but users should know about issues
- [ ] **Don't recommend yet** - Needs more work before wider testing

### Additional Comments
_Any other feedback, suggestions, or observations:_

---

## Submission

**How to submit your results**:

1. **GitHub Issue**: [Create a new issue](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/issues) with your results
2. **Email**: Send to the development team
3. **Discussion**: Use [GitHub Discussions](https://github.com/EOPF-Sample-Service/GDAL-ZARR-EOPF/discussions)

**Title your report**: "Test Results - [Your OS] - [Pass/Fail/Partial]"

Example: "Test Results - macOS M3 - Pass with Error 126"

---

**Thank you for testing!** Your feedback helps make this plugin better for everyone.

**Test Date**: ___________  
**Tester**: ___________  
**Plugin Version**: ___________
