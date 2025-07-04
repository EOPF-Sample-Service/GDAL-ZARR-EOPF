# Contributing to EOPF-Zarr GDAL Plugin

Thank you for your interest in contributing to the EOPF-Zarr GDAL plugin! This document provides guidelines for contributing to the project.

## Code of Conduct

We are committed to providing a welcoming and inclusive environment for all contributors. Please be respectful and constructive in all interactions.

## Getting Started

### Prerequisites
- GDAL 3.4.0 or higher
- CMake 3.16 or higher
- C++ compiler with C++14 support
- Git for version control

### Development Setup

1. **Fork and Clone**
   ```bash
   git clone https://github.com/YOUR_USERNAME/GDAL-ZARR-EOPF.git
   cd GDAL-ZARR-EOPF
   ```

2. **Set up Development Environment**
   ```bash
   # Install dependencies (Ubuntu/Debian)
   sudo apt-get install libgdal-dev cmake build-essential
   
   # Create development build
   mkdir build-dev
   cd build-dev
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   make
   ```

3. **Verify Setup**
   ```bash
   # Run tests
   make test
   
   # Check plugin loads
   gdalinfo --formats | grep -i eopf
   ```

## Development Workflow

### Branch Strategy
- `main`: Stable, production-ready code
- `develop`: Integration branch for ongoing development
- `feature/*`: Individual feature development
- `bugfix/*`: Bug fixes
- `hotfix/*`: Critical production fixes

### Creating Features

1. **Create Feature Branch**
   ```bash
   git checkout develop
   git pull origin develop
   git checkout -b feature/your-feature-name
   ```

2. **Implement Changes**
   - Write code following project conventions
   - Add or update tests
   - Update documentation

3. **Test Changes**
   ```bash
   # Build and test
   cd build-dev
   make && make test
   
   # Test with real data
   gdalinfo sample_data/test.zarr
   ```

4. **Commit Changes**
   ```bash
   # Stage changes
   git add .
   
   # Commit with sign-off (required)
   git commit -s -m "feat: add support for new metadata format
   
   - Parse additional EOPF metadata attributes
   - Add validation for required fields
   - Update tests for new functionality"
   ```

5. **Push and Create PR**
   ```bash
   git push origin feature/your-feature-name
   # Create pull request via GitHub UI
   ```

## Commit Guidelines

### Commit Message Format
Please sign off your commits with `git commit -s`.

Use conventional commit format:
```
<type>(<scope>): <description>

<body>

<footer>
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, etc.)
- `refactor`: Code refactoring
- `test`: Adding or updating tests
- `chore`: Maintenance tasks

**Examples:**
```bash
git commit -s -m "feat(driver): add support for zarr v3 format"
git commit -s -m "fix(metadata): handle missing coordinate attributes"
git commit -s -m "docs: update installation guide for macOS"
```

## Coding Standards

### C++ Guidelines
- Follow GDAL coding conventions
- Use meaningful variable and function names
- Add comments for complex logic
- Handle errors appropriately
- Follow RAII principles for resource management

### Code Style
```cpp
// Class names: PascalCase
class EOPFDataset : public GDALDataset {
    // Member variables: m_ prefix
    std::string m_zarrPath;
    
    // Public methods: PascalCase
    CPLErr GetGeoTransform(double* padfTransform) override;
    
    // Private methods: camelCase
    bool parseMetadata();
};

// Function names: PascalCase for public, camelCase for private
CPLErr EOPFDataset::GetGeoTransform(double* padfTransform) {
    // Local variables: camelCase
    const char* projectionRef = GetProjectionRef();
    
    // Constants: UPPER_CASE
    const int DEFAULT_CHUNK_SIZE = 256;
}
```

### Error Handling
```cpp
// Use GDAL error reporting
if (dataset == nullptr) {
    CPLError(CE_Failure, CPLE_OpenFailed, 
             "Failed to open dataset: %s", filename);
    return nullptr;
}

// Check return values
CPLErr result = someOperation();
if (result != CE_None) {
    return result;
}
```

## Testing

### Unit Tests
- Add tests for new functionality
- Test edge cases and error conditions
- Use descriptive test names

```cpp
TEST(EOPFDatasetTest, OpenValidZarrDataset) {
    auto dataset = std::unique_ptr<GDALDataset>(
        GDALOpen("test_data/valid.zarr", GA_ReadOnly));
    ASSERT_NE(dataset, nullptr);
    EXPECT_GT(dataset->GetRasterCount(), 0);
}

TEST(EOPFDatasetTest, HandleMissingMetadata) {
    auto dataset = std::unique_ptr<GDALDataset>(
        GDALOpen("test_data/no_metadata.zarr", GA_ReadOnly));
    EXPECT_EQ(dataset, nullptr);
}
```

### Integration Tests
- Test with real EOPF datasets
- Verify GDAL integration
- Test cloud access functionality

### Testing Best Practices
- Tests should be fast and reliable
- Use temporary files for file system tests
- Mock external dependencies when possible
- Clean up resources after tests

## Documentation

### Required Documentation
- Update relevant `.md` files for user-facing changes
- Add inline code comments for complex logic
- Update API documentation for public interfaces

### Documentation Types
- **User Documentation**: How to use new features
- **Developer Documentation**: Implementation details
- **API Documentation**: Public interface specifications

## Pull Request Process

### Before Submitting
- [ ] Code builds without errors or warnings
- [ ] All tests pass
- [ ] Code follows project style guidelines
- [ ] Documentation is updated
- [ ] Commit messages follow conventions
- [ ] Commits are signed off

### PR Description Template
Use the provided PR template and include:
- Clear description of changes
- Motivation and context
- Testing performed
- Breaking changes (if any)
- Related issues

### Review Process
1. **Automated Checks**: CI/CD pipeline must pass
2. **Code Review**: At least one approval required
3. **Testing**: Reviewer may test changes locally
4. **Documentation Review**: Ensure docs are accurate

### After Approval
- PRs are merged using "Squash and merge"
- Feature branches are deleted after merge
- Release notes updated if necessary

## Issue Reporting

### Bug Reports
Please include:
- Operating system and version
- GDAL version
- Plugin version
- Steps to reproduce
- Expected vs. actual behavior
- Sample data (if possible)
- Log output with `CPL_DEBUG=ON`

### Feature Requests
Please include:
- Use case description
- Proposed solution (if any)
- Alternative solutions considered
- Additional context

## Getting Help

### Resources
- [User Guide](docs/user-guide.md)
- [API Documentation](docs/api.md)
- [FAQ](docs/faq.md)
- [Architecture Documentation](docs/architecture.md)

### Communication
- GitHub Issues: Bug reports and feature requests
- GitHub Discussions: General questions and ideas
- Pull Request Comments: Code-specific discussions

## Recognition

Contributors will be acknowledged in:
- Release notes
- `CONTRIBUTORS.md` file
- GitHub contributor statistics

Thank you for contributing to the EOPF-Zarr GDAL plugin!