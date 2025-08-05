# Contributing to NOT Gate Game

## Getting Started

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## Development Guidelines

### Code Style
- Follow the `.clang-format` configuration
- Use meaningful variable/function names
- Comment complex algorithms
- 한국어 주석 OK for complex logic

### Performance First
- Profile before optimizing
- Use appropriate data structures
- Minimize allocations in hot paths
- Test with large circuits (100k+ gates)

### Git Commit Messages
- Use clear, descriptive messages
- Reference issue numbers when applicable
- Keep commits small and focused

Example:
```
Add batch rendering for gates

- Implement instanced rendering
- Reduce draw calls from 1000 to 10
- Improves performance by 5x for large circuits

Fixes #123
```

### Testing
- Add tests for new features
- Ensure all tests pass before PR
- Include performance benchmarks for optimizations

### Documentation
- Update relevant documentation
- Add inline comments for complex code
- Update CLAUDE.md if adding major features

## Pull Request Process

1. Update documentation
2. Add/update tests
3. Ensure code follows style guide
4. Update ROADMAP.md progress
5. Request review

## Areas Needing Contribution

- Level design
- UI/UX improvements
- Performance optimizations
- Platform-specific fixes
- Documentation translations