# AGENTS.md - IDLC Java Generator Plugin

## Project Overview

This is a CycloneDDS IDLC compiler plugin that generates Java 17+ code from IDL files. The plugin is written in C11 and integrates with the CycloneDDS IDL compiler.

## Build Commands

### Full Build
```bash
cd /home/stuart/repos/idlc2java
mkdir -p build && cd build
cmake -DCYCLONEDDS_ROOT=/opt/cyclonedds ..
make
```

### Install Plugin
```bash
cp build/idlc_java.so /opt/cyclonedds/lib/libcycloneddsidljava.so
```

### Running Tests
```bash
cd /home/stuart/repos/idlc2java/build
make test
# or
./tests/run_tests
```

### Test the Plugin Manually
```bash
# Compile a simple IDL file
/opt/cyclonedds/bin/idlc -l java -o /tmp/output examples/shapes.idl

# Or with XTypes
/opt/cyclonedds/bin/idlc -DDDS_XTYPES -l java -o /tmp/output examples/shapes.idl
```

## Code Style Guidelines

### C Standard
- Use C11 standard (`set(CMAKE_C_STANDARD 11)`)
- Use stdbool.h for boolean types, not int/0/1

### File Organization
- Header files in `include/`
- Source files in `src/`
- Each .c file should have a corresponding header if functions are exported
- Test files in `tests/`

### Naming Conventions
- **Types**: Use `_t` suffix for typedef structs (e.g., `generator_state_t`)
- **Functions**: `lower_case_with_underscores`
- **Constants/Macros**: `UPPER_CASE_WITH_UNDERSCORES`
- **Variables**: `lower_case_with_underscores`
- **Struct/Enum members**: `lower_case_with_underscores`
- **Files**: `lower_case_with_underscores.c`, `lower_case_with_underscores.h`

### Header File Conventions
```c
#ifndef IDL_PLUGIN_NAME_H
#define IDL_PLUGIN_NAME_H

#include <std headers>
#include "local headers"

#ifdef __cplusplus
extern "C" {
#endif

/* Declarations */

#ifdef __cplusplus
}
#endif

#endif /* IDL_PLUGIN_NAME_H */
```

### Include Order
1. Standard C library headers (`<stdio.h>`, `<stdlib.h>`, etc.)
2. System headers (`<sys/stat.h>`, etc.)
3. Third-party headers (`"idlc/generator.h"`, `"idl/tree.h"`)
4. Local project headers (`"idlc_java.h"`)

### Function Declarations
- Use `extern` for functions defined in one .c and used in another
- Return `int` for functions that can fail: `0` = success, `-1` = error
- Use `bool` for boolean parameters and return values
- Prefer `const char *` for string inputs that won't be modified

### Memory Management
- Always free allocated memory before returning error
- Check return values from malloc/calloc/realloc
- Use `strdup()` for string duplication (remember to free)

### Code Structure

#### Generator Functions
```c
// State structure for traversal
typedef struct generator_state {
    const char *output_dir;
    const char *package_prefix;
    int errors;
    /* ... other state ... */
} generator_state_t;

// Main entry point (must be named 'generate')
int generate(const idl_pstate_t *pstate, const idlc_generator_config_t *config) {
    // Validate inputs
    if (!pstate || !pstate->root) {
        fprintf(stderr, "Error: No parsed IDL available\n");
        return -1;
    }
    
    // Initialize state
    generator_state_t state = {
        .output_dir = config->output_dir,
        /* ... other fields ... */
    };
    
    // Process AST
    process_node((idl_node_t *)pstate->root, &state);
    
    return state.errors;
}
```

#### AST Traversal
```c
static void process_node(idl_node_t *node, generator_state_t *state) {
    if (!node) return;
    
    idl_mask_t mask = idl_mask(node);
    
    if (mask & IDL_STRUCT) {
        idl_struct_t *struct_def = (idl_struct_t *)node;
        // Process struct
    }
    else if (mask & IDL_ENUM) {
        idl_enum_t *enum_def = (idl_enum_t *)node;
        // Process enum
    }
    else if (mask & IDL_MODULE) {
        idl_module_t *module = (idl_module_t *)module;
        // Process children
        if (module->definitions) {
            idl_node_t *def = (idl_node_t *)module->definitions;
            while (def) {
                process_node(def, state);
                def = def->next;
            }
        }
    }
    
    // Continue to next sibling
    if (node->next) {
        process_node(node->next, state);
    }
}
```

#### Accessing IDL Names
```c
// Correct way to access names from AST nodes
const char *struct_name = "GeneratedStruct";
if (struct_def->name && struct_def->name->identifier) {
    struct_name = struct_def->name->identifier;
}

// Member names from declarators
const char *member_name = "field";
if (member->declarators && member->declarators->name && 
    member->declarators->name->identifier) {
    member_name = member->declarators->name->identifier;
}
```

### String Building
- Use the project's `string_builder_t` for building output strings
- Available functions:
  - `sb_create()` - Create a new string builder
  - `sb_destroy()` - Free the string builder
  - `sb_append()` - Append a string
  - `sb_appendf()` - Append formatted string
  - `sb_string()` - Get the final string

### Error Handling
- Print errors to `stderr`, not `stdout`
- Use `fprintf(stderr, "Error: %s\n", message)`
- Return non-zero on failure
- Count errors and return total at end

### Java Code Generation

#### Records (Structs)
```java
package <package>;

import java.io.Serializable;

public record <ClassName>(
    <type1> <field1>,
    <type2> <field2>
) implements Serializable {

    @Override
    public String toString() {
        return "<ClassName>[field1=" + field1() + ", field2=" + field2() + "]";
    }
}
```

#### Enums
```java
package <package>;

public enum <EnumName> {
    VALUE1(0),
    VALUE2(1);

    private final int value;

    <EnumName>(int value) {
        this.value = value;
    }

    public int getValue() {
        return value;
    }
}
```

#### Typedefs (Wrapper Classes)
```java
package <package>;

public class <TypeName> {
    private <wrapped_type> value;

    public <TypeName>() { }

    public <TypeName>(<wrapped_type> value) {
        this.value = value;
    }

    public <wrapped_type> getValue() { return value; }
    public void setValue(<wrapped_type> value) { this.value = value; }
}
```

## CycloneDDS Integration

### Plugin Requirements
- Export a function named `generate` with signature:
  ```c
  int generate(const idl_pstate_t *pstate, const idlc_generator_config_t *config);
  ```
- The plugin must be named `libcycloneddsidljava.so` and placed in `/opt/cyclonedds/lib/`

### Important IDL Types
- `idl_struct_t` - Struct definitions
- `idl_enum_t` - Enum definitions  
- `idl_typedef_t` - Typedef definitions
- `idl_module_t` - Module definitions
- `idl_member_t` - Struct members
- `idl_enumerator_t` - Enum values
- `idl_type_spec_t` - Type specifications

### Key Masks
- `IDL_STRUCT` (0x20000)
- `IDL_ENUM` (0x10000)
- `IDL_MODULE` (0x400000000)
- `IDL_TYPEDEF`

## Testing

### Test IDL Files
- Keep test IDL files in `examples/` directory
- Use simple, valid IDL constructs
- Avoid CycloneDDS unsupported features:
  - `wchar` / `wstring` (use `char` / `string`)
  - Reserved words as typedef names (e.g., `Int32`, `UInt32`)
  - Some complex constant definitions
  - Interfaces (`interface` keyword)
  - Exceptions (`exception` keyword)

### Manual Testing
```bash
# Test basic struct/enum/typedef generation
/opt/cyclonedds/bin/idlc -l java -o /tmp/test examples/shapes.idl

# Test with includes
/opt/cyclonedds/bin/idlc -l java -o /tmp/test -I examples examples/shapes.idl
```

## Common Issues

1. **Name extraction**: Use `node->name->identifier` directly, NOT `idl_identifier()` which can cause segfaults
2. **Package paths**: Use full module path (walk parent chain) for nested modules
3. **Directory creation**: Create parent directories for nested packages using mkdir loop
4. **Output directory**: Handle empty string case for `-o .`
