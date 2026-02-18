# Building the IDLC Java Generator Plugin

## Prerequisites

- **CycloneDDS** installed at `/opt/cyclonedds` (or set `CYCLONEDDS_ROOT`)
- **CMake** 3.15+
- **GCC/Clang** with C11 support

## Quick Build

```bash
# Clone and build
mkdir build && cd build
cmake -DCYCLONEDDS_ROOT=/opt/cyclonedds ..
make

# Install plugin
cp idlc_java.so.1.0.0 /opt/cyclonedds/lib/libcycloneddsidljava.so
```

## Build Options

| Option | Description | Default |
|--------|-------------|---------|
| `CYCLONEDDS_ROOT` | Path to CycloneDDS installation | `/opt/cyclonedds` |
| `CMAKE_BUILD_TYPE` | Build type (Debug/Release) | Release |

## Directory Structure

```
idlc2java/
├── src/
│   ├── generator.c       # Main generator entry point
│   ├── java_record.c    # Struct/union/bitmask generation
│   ├── java_type.c      # Type mapping utilities
│   ├── package_resolver.c # IDL module → Java package
│   ├── annotation.c     # Annotation handling
│   └── string_builder.c # String buffer utilities
├── include/              # Header files
├── tests/               # Test suite
├── examples/            # Example IDL files
│   ├── all-types/       # Comprehensive test cases
│   └── tex/            # TEX domain profile
└── CMakeLists.txt       # Build configuration
```

## Plugin Installation

The plugin must be named `libcycloneddsidljava.so` and placed in the CycloneDDS lib directory:

```bash
cp build/idlc_java.so.1.0.0 /opt/cyclonedds/lib/libcycloneddsidljava.so
```

## Usage

### Basic Generation

```bash
# Generate Java from IDL
/opt/cyclonedds/bin/idlc -l java -o output input.idl

# With XTypes support
/opt/cyclonedds/bin/idlc -DDDS_XTYPES -l java -o output input.idl

# With include paths
/opt/cyclonedds/bin/idlc -l java -o output -I include_dir input.idl
```

### Options

| Option | Description |
|--------|-------------|
| `-l java` | Select Java generator plugin |
| `-o <dir>` | Output directory |
| `-I <dir>` | Include path for IDL imports |
| `-DDDS_XTYPES` | Enable XTypes support |

## Testing

```bash
# Run test suite
cd build
make test

# Or run tests directly
./tests/run_tests
```

### Test Coverage

- Plugin loading
- Struct generation (extends Structure, @FieldOrder)
- Serialize/deserialize methods
- describeType() for DDS registration
- Enum generation (Java enum with getValue())
- Bitmask generation (extends Structure)
- Union generation
- Sequence types (java.util.List)
- Cross-module typedefs
- Complex IDL files (TEX EntityPayload)

## Generated Code Structure

### Structs (JNA Structure)

```java
public class Point extends Structure {
    @Structure.FieldOrder({"x", "y"})
    public int x;
    public int y;

    public byte[] serialize() { /* CDR encoding */ }
    public void deserialize(byte[] data) { /* CDR decoding */ }
    public static DynamicType describeType() { /* DDS type info */ }
}
```

### Enums (Java Enum)

```java
public enum ShapeType {
    CIRCLE(0),
    RECTANGLE(1);
    
    private final int value;
    public int getValue() { return value; }
}
```

### Bitmasks (JNA Structure)

```java
public class Flags extends Structure {
    public long value;
    public static final long FLAG_READ = 1L << 0;
    public boolean isSet(long flag) { return (value & flag) == flag; }
}
```

## Type Mapping

| IDL Type | Java Type |
|----------|-----------|
| `boolean` | `boolean` |
| `octet`, `char` | `byte` |
| `short` | `short` |
| `unsigned short` | `char` (note: should be `short`) |
| `long` | `int` |
| `unsigned long` | `int` |
| `long long` | `long` |
| `float` | `float` |
| `double` | `double` |
| `string` | `String` |
| `sequence<T>` | `List<T>` |

## Development

### Code Style

- C11 standard
- Types: `struct_name_t` suffix
- Functions: `lower_case_with_underscores`
- Variables: `lower_case_with_underscores`

### Adding New Features

1. Add type detection in `java_type.c`
2. Add generation logic in `java_record.c`
3. Add test case in `examples/`
4. Verify with `make test`

## Troubleshooting

### Plugin Not Found

Ensure the plugin is installed at the correct path:
```bash
ls -la /opt/cyclonedds/lib/libcycloneddsidljava.so
```

### IDL Parse Errors

Make sure to use correct include paths:
```bash
idlc -l java -I /path/to/includes input.idl
```

### Memory Issues

Run with valgrind for debugging:
```bash
valgrind --leak-check=full idlc -l java -o output input.idl
```
