# IDLC Java Generator Plugin

A CycloneDDS IDLC compiler plugin that generates Java 17+ code from IDL files with CDR serialization support.

## Features

- **Java 17+ Records**: Generates modern, immutable record classes
- **CDR Serialization**: Built-in Common Data Representation (CDR) format support for DDS interoperability
- **Package Mapping**: IDL modules map directly to Java packages
- **Type Support**: Structs, enums, unions, bitmasks, sequences, arrays
- **Annotations**: Full support for DDS annotations (@key, @optional, @topic, @nested, etc.)

## Building

### Prerequisites

- CycloneDDS installed at `/opt/cyclonedds` (or set `CYCLONEDDS_ROOT`)
- CMake 3.15+
- GCC/Clang with C11 support

### Build Steps

```bash
mkdir build && cd build
cmake -DCYCLONEDDS_ROOT=/opt/cyclonedds ..
make
```

### Install

```bash
make install
```

This installs `idlc_java.so` to `/opt/cyclonedds/lib/idlc/`.

## Usage

### Basic Usage

```bash
idlc -l java -o output/path input.idl
```

### Options

- `-l java` - Select the Java generator plugin
- `-o <dir>` - Output directory for generated Java files
- `-I <dir>` - Include path for IDL imports
- `-DDDS_XTYPES` - Use XTypes for type evolution support

### Example

Given an IDL file `examples/shapes.idl`:

```idl
module Shapes {
    struct Point {
        @key long x;
        @key long y;
    };
    
    struct Circle {
        @key long id;
        Point center;
        double radius;
        string color;
    };
    
    enum ShapeType {
        CIRCLE,
        RECTANGLE,
        TRIANGLE
    };
};
```

Generate Java code:

```bash
/opt/cyclonedds/bin/idlc -l java -o generated examples/shapes.idl
```

This generates:

```
generated/
└── Shapes/
    ├── Point.java
    ├── Circle.java
    └── ShapeType.java
```

## Generated Code Structure

### Records for Structs

```java
package Shapes;

import org.omg.dds.core.DDSObject;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public record Circle(
    @Key long id(),
    Point center(),
    double radius(),
    String color()
) implements DDSObject {
    
    public static Circle readCDR(InputStream input) throws IOException {
        // CDR deserialization
    }
    
    public void writeCDR(OutputStream output) throws IOException {
        // CDR serialization
    }
}
```

### Enums

```java
package Shapes;

public enum ShapeType {
    CIRCLE(0),
    RECTANGLE(1),
    TRIANGLE(2);
    
    private final int value;
    
    ShapeType(int value) {
        this.value = value;
    }
    
    public int getValue() {
        return value;
    }
}
```

## Type Mapping

| IDL Type | Java Type |
|----------|-----------|
| `boolean` | `boolean` |
| `octet`, `char` | `byte` |
| `short`, `unsigned short` | `short`, `char` |
| `long`, `unsigned long` | `int` |
| `long long`, `unsigned long long` | `long` |
| `float` | `float` |
| `double` | `double` |
| `string`, `wstring` | `String` |
| `sequence<T>` | `List<T>` |
| `struct T` | `T` (record) |
| `enum T` | `T` (enum) |

## Testing

Run the test suite:

```bash
cd build
make test
```

Or manually:

```bash
./tests/run_tests
```

## License

This plugin follows the same license as CycloneDDS (Eclipse Public License 2.0).

## Contributing

Contributions are welcome! Please ensure:

1. Code follows C11 standards
2. All tests pass
3. Generated Java code compiles with Java 17+
4. CDR serialization is correct

## TODO

- [ ] Complete union generation
- [ ] Complete bitmask generation
- [ ] Add support for arrays
- [ ] Implement full CDR serialization
- [ ] Add unit tests for CDR encoding/decoding
- [ ] Support for bounded sequences
- [ ] Support for fixed-point types
- [ ] Add Maven/Gradle build for generated code
