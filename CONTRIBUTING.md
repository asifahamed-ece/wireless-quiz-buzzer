# Contributing to Wireless Quiz Buzzer

First off, thank you for considering contributing to this project! 🎉

## Table of Contents
- [Code of Conduct](#code-of-conduct)
- [How Can I Contribute?](#how-can-i-contribute)
- [Development Process](#development-process)
- [Style Guidelines](#style-guidelines)
- [Commit Messages](#commit-messages)

## Code of Conduct

This project adheres to a [Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code.

## How Can I Contribute?

### Reporting Bugs
Before creating bug reports, please check existing issues. When creating a bug report, include:

- **Clear title and description**
- **Steps to reproduce**
- **Expected vs actual behavior**
- **Screenshots** (if applicable)
- **Hardware/software versions**

**Bug Report Template:**
```markdown
**Describe the bug**
A clear description of what the bug is.

**To Reproduce**
Steps to reproduce the behavior:
1. Go to '...'
2. Click on '...'
3. See error

**Expected behavior**
What you expected to happen.

**System Info:**
 - ESP32 Board: [e.g. DevKit v1]
 - Firmware Version: [e.g. 1.0.0]
 - Browser: [e.g. Chrome 120]
```

### Suggesting Features
Feature requests are welcome! Please provide:

- **Clear use case**
- **Expected behavior**
- **Possible implementation** (if you have ideas)
- **Alternatives considered**

### Pull Requests

1. **Fork the repo** and create your branch from `main`
2. **Make your changes** with clear commits
3. **Test thoroughly** on actual hardware
4. **Update documentation** if needed
5. **Submit pull request** with description

## Development Process

### Setting Up Development Environment

```bash
# Clone your fork
git clone https://github.com/YOUR_USERNAME/wireless-quiz-buzzer.git

# Add upstream remote
git remote add upstream https://github.com/asifahamed-ece/wireless-quiz-buzzer.git

# Create feature branch
git checkout -b feature/amazing-feature

# Keep your branch updated
git fetch upstream
git rebase upstream/main
```

### Testing Requirements

Before submitting, ensure:
- ✅ Master ESP32 boots correctly
- ✅ Slave ESP32 connects to master
- ✅ Dashboard loads without errors
- ✅ LISTEN → READY → ANSWERED flow works
- ✅ Audio feedback plays correctly
- ✅ Battery monitoring displays properly
- ✅ No console errors in browser

### Code Style Guidelines

#### C++ (ESP32 Firmware)
```cpp
// Use descriptive variable names
int connectedTeams = 0;  // Good

// Comment complex logic
if(voltage > 3.7) {
  zone = 2;  // Green: Healthy
}

// Use consistent formatting
void functionName() {
  if(condition) {
    // code
  }
}
```

#### JavaScript (Dashboard)
```javascript
// Use modern ES6+ syntax
const updateDisplay = (data) => {
  // Arrow functions preferred
};

// Clear variable names
let currentPhase = "LISTEN";  // Good
```

## Commit Messages

### Format
```
Type: Brief description (50 chars or less)

More detailed explanation if needed (wrap at 72 chars).
- Bullet points okay
- Use present tense

Fixes #123
```

### Types
- **Add:** New feature
- **Fix:** Bug fix
- **Update:** Modify existing feature
- **Remove:** Delete code/feature
- **Refactor:** Code restructuring
- **Docs:** Documentation only
- **Style:** Formatting, no code change
- **Test:** Add/modify tests

### Examples
```
Add: Two-phase quiz system (LISTEN/READY)

Implemented phase-based quiz flow:
- LISTEN: Question being read, buzzers ignored
- READY: Buzzers active, BGM playing

Fixes #42
```

## Branch Naming

- `feature/` - New features
- `fix/` - Bug fixes
- `docs/` - Documentation
- `refactor/` - Code improvements

## Questions?

Feel free to:
- Open an issue with `question` label
- Email: asifahamed670@gmail.com

---

**Thank you for contributing! 🎉**
