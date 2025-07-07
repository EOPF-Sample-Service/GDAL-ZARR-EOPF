---
name: Test User Report
about: Report your testing results for the EOPF Zarr GDAL plugin
title: 'Test Results - [OS] - [Pass/Fail/Partial]'
labels: testing, user-feedback
assignees: ''

---

## Test Environment
**Operating System**: (e.g., Windows 11, macOS 14, Ubuntu 22.04)
**Architecture**: (e.g., x86_64, arm64)
**GDAL Version**: (run `gdalinfo --version`)

## Installation Results
- [ ] Plugin installed successfully
- [ ] Plugin appears in `gdalinfo --formats | grep EOPFZARR`
- [ ] Installation script completed without fatal errors

**Installation method used**: (e.g., `install-windows.bat`, `./install-macos.sh`)

## Functionality Test Results
- [ ] Plugin loads without fatal errors
- [ ] Can open Zarr datasets (if test data available)
- [ ] Works in QGIS (if tested)
- [ ] Works with Python GDAL (if tested)

## Issues Encountered
**Error messages** (copy/paste any error output):
```
[Paste error messages here]
```

**Describe any problems**:
[Describe what didn't work as expected]

## What Worked Well
[Describe what worked correctly]

## Overall Assessment
- [ ] **Working** - Plugin functions correctly for basic use
- [ ] **Working with caveats** - Functions but has harmless errors/warnings
- [ ] **Partially working** - Some features work, others don't
- [ ] **Not working** - Cannot use plugin due to major issues

## Additional Information
**Harmless warnings noticed** (e.g., Windows Error 126):
[Note any warnings that don't prevent functionality]

**Performance observations**:
[Any notes about speed, memory usage, etc.]

**Suggestions for improvement**:
[Any feedback for making the plugin better]

## Recommendation
- [ ] Ready for other test users
- [ ] Ready with documentation of known issues
- [ ] Needs more work before wider testing

## Test Data Used
- [ ] No test data available
- [ ] Used provided sample data
- [ ] Used own Zarr datasets
- [ ] Used remote/cloud datasets

**Brief description of test data**:
[What kind of datasets did you test with?]

---

**Thank you for testing!** Your feedback helps improve the plugin for everyone.
