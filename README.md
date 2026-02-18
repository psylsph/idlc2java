# IDLC Java Generator Plugin

A **CycloneDDS IDLC compiler plugin** that generates production-ready Java 17+ code from IDL files with built-in CDR serialization support. Perfect for building DDS-based Java applications with CycloneDDS.

## Why This Plugin?

- 🚀 **Zero-Copy Performance** - JNA Structure classes for efficient memory mapping
- 📦 **Out-of-the-Box** - Built-in CDR serialization/deserialization
- 🔒 **Type-Safe** - Full type mapping with compile-time safety
- 🎯 **XTypes Ready** - DynamicType support for DDS type registration
- ⚡ **Production-Ready** - Used in real-world DDS applications

## Key Features

- **JNA Integration** - Generated classes extend `com.sun.jna.Structure`
- **CDR Serialization** - Native Common Data Representation (CDR) encoding
- **Full Type Support** - Structs, enums, unions, bitmasks, sequences
- **Module Mapping** - IDL modules → Java packages automatically
- **Cross-Module Types** - References to types in other modules work seamlessly

## Quick Start

```bash
# Install plugin
cp build/idlc_java.so /opt/cyclonedds/lib/libcycloneddsidljava.so

# Generate Java from IDL
/opt/cyclonedds/bin/idlc -l java -o output input.idl
```

## Generated Code Example

```java
// IDL
struct Point {
    @key long x;
    @key long y;
};

// Generated Java (JNA Structure)
public class Point extends Structure {
    @Structure.FieldOrder({"x", "y"})
    public int x;
    public int y;

    public byte[] serialize() { /* CDR encoding */ }
    public void deserialize(byte[] data) { /* CDR decoding */ }
    
    public static DynamicType describeType() { /* DDS type info */ }
}
```

## Related Projects

This plugin is part of the CycloneDDS ecosystem:
- [eclipse-cyclone/cycloneDDS](https://github.com/eclipse-cyclone/cycloneDDS) - The main CycloneDDS project
- [eclipse-cyclone/cycloneDDS-IDL](https://github.com/eclipse-cyclone/cycloneDDS-IDL) - IDL compiler

## License

Licensed under **Eclipse Public License 2.0 (EPL-2.0)** - the same license as CycloneDDS.

---

For build instructions and development details, see [BUILD.md](BUILD.md).
