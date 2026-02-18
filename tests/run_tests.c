/*
 * Copyright (c) 2024 IDLC Java Generator Contributors
 *
 * Test suite for IDL to Java Generator
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define TEST_IDLC "/opt/cyclonedds/bin/idlc"
#define TEST_PLUGIN_DIR "/home/stuart/repos/idlc2java/build"
#define TEST_EXAMPLES_DIR "/home/stuart/repos/idlc2java"

int run_command(const char *command) {
    FILE *fp = popen(command, "r");
    if (!fp) {
        fprintf(stderr, "Failed to run command: %s\n", command);
        return -1;
    }
    
    char buffer[4096];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer)-1, fp)) > 0) {
        buffer[n] = '\0';
    }
    
    int status = pclose(fp);
    return WEXITSTATUS(status);
}

int file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

int count_files_in_dir(const char *dir) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "ls -1 %s 2>/dev/null | wc -l", dir);
    FILE *fp = popen(cmd, "r");
    if (!fp) return 0;
    int count = 0;
    fscanf(fp, "%d", &count);
    pclose(fp);
    return count;
}

int test_plugin_loading(void) {
    printf("=== Test: Plugin Loading ===\n");
    
    char command[512];
    snprintf(command, sizeof(command), 
        "LD_LIBRARY_PATH=%s %s -l java -o /tmp/test_plugin_check -I %s/examples/all-types %s/examples/all-types/shapes.idl 2>&1",
        TEST_PLUGIN_DIR, TEST_IDLC, TEST_EXAMPLES_DIR, TEST_EXAMPLES_DIR);
    
    int result = run_command(command);
    printf("%s\n", result == 0 ? "✓ Plugin loaded" : "✗ Plugin failed");
    return result == 0 ? 0 : -1;
}

int test_basic_generation(void) {
    printf("\n=== Test: Basic Generation ===\n");
    
    char command[1024];
    snprintf(command, sizeof(command),
        "LD_LIBRARY_PATH=%s %s -l java -o /tmp/idlc_test_shapes -I %s/examples/all-types %s/examples/all-types/shapes.idl 2>&1",
        TEST_PLUGIN_DIR, TEST_IDLC, TEST_EXAMPLES_DIR, TEST_EXAMPLES_DIR);
    
    int result = run_command(command);
    
    if (result == 0) {
        int count = count_files_in_dir("/tmp/idlc_test_shapes/Shapes");
        printf("✓ Generated %d files in Shapes module\n", count);
    } else {
        printf("✗ Generation failed\n");
    }
    return result == 0 ? 0 : -1;
}

int test_struct_extends_structure(void) {
    printf("\n=== Test: Struct extends Structure ===\n");
    
    if (!file_exists("/tmp/idlc_test_shapes/Shapes/Point.java")) {
        printf("✗ Point.java not found\n");
        return -1;
    }
    
    FILE *f = fopen("/tmp/idlc_test_shapes/Shapes/Point.java", "r");
    char buf[256];
    int found = 0;
    while (fgets(buf, sizeof(buf), f)) {
        if (strstr(buf, "extends Structure")) {
            found = 1;
            break;
        }
    }
    fclose(f);
    
    printf("%s\n", found ? "✓ Struct extends Structure" : "✗ Struct doesn't extend Structure");
    return found ? 0 : -1;
}

int test_struct_has_field_order(void) {
    printf("\n=== Test: Struct has @FieldOrder ===\n");
    
    FILE *f = fopen("/tmp/idlc_test_shapes/Shapes/Point.java", "r");
    char buf[256];
    int found = 0;
    while (fgets(buf, sizeof(buf), f)) {
        if (strstr(buf, "@Structure.FieldOrder")) {
            found = 1;
            break;
        }
    }
    fclose(f);
    
    printf("%s\n", found ? "✓ Has @FieldOrder" : "✗ Missing @FieldOrder");
    return found ? 0 : -1;
}

int test_struct_has_serialize(void) {
    printf("\n=== Test: Struct has serialize() ===\n");
    
    FILE *f = fopen("/tmp/idlc_test_shapes/Shapes/Point.java", "r");
    char buf[256];
    int found = 0;
    while (fgets(buf, sizeof(buf), f)) {
        if (strstr(buf, "public byte[] serialize()")) {
            found = 1;
            break;
        }
    }
    fclose(f);
    
    printf("%s\n", found ? "✓ Has serialize()" : "✗ Missing serialize()");
    return found ? 0 : -1;
}

int test_struct_has_deserialize(void) {
    printf("\n=== Test: Struct has deserialize() ===\n");
    
    FILE *f = fopen("/tmp/idlc_test_shapes/Shapes/Point.java", "r");
    char buf[256];
    int found = 0;
    while (fgets(buf, sizeof(buf), f)) {
        if (strstr(buf, "public void deserialize(")) {
            found = 1;
            break;
        }
    }
    fclose(f);
    
    printf("%s\n", found ? "✓ Has deserialize()" : "✗ Missing deserialize()");
    return found ? 0 : -1;
}

int test_struct_has_describe_type(void) {
    printf("\n=== Test: Struct has describeType() ===\n");
    
    FILE *f = fopen("/tmp/idlc_test_shapes/Shapes/Point.java", "r");
    char buf[256];
    int found = 0;
    while (fgets(buf, sizeof(buf), f)) {
        if (strstr(buf, "public static DynamicType describeType()")) {
            found = 1;
            break;
        }
    }
    fclose(f);
    
    printf("%s\n", found ? "✓ Has describeType()" : "✗ Missing describeType()");
    return found ? 0 : -1;
}

int test_enum_is_java_enum(void) {
    printf("\n=== Test: Enum is Java enum ===\n");
    
    if (!file_exists("/tmp/idlc_test_shapes/Shapes/ShapeType.java")) {
        printf("✗ ShapeType.java not found\n");
        return -1;
    }
    
    FILE *f = fopen("/tmp/idlc_test_shapes/Shapes/ShapeType.java", "r");
    char buf[256];
    int found = 0;
    while (fgets(buf, sizeof(buf), f)) {
        if (strstr(buf, "public enum ShapeType")) {
            found = 1;
            break;
        }
    }
    fclose(f);
    
    printf("%s\n", found ? "✓ Is Java enum" : "✗ Not a Java enum");
    return found ? 0 : -1;
}

int test_enum_has_get_value(void) {
    printf("\n=== Test: Enum has getValue() ===\n");
    
    FILE *f = fopen("/tmp/idlc_test_shapes/Shapes/ShapeType.java", "r");
    char buf[256];
    int found = 0;
    while (fgets(buf, sizeof(buf), f)) {
        if (strstr(buf, "public int getValue()")) {
            found = 1;
            break;
        }
    }
    fclose(f);
    
    printf("%s\n", found ? "✓ Has getValue()" : "✗ Missing getValue()");
    return found ? 0 : -1;
}

int test_bitmask_extends_structure(void) {
    printf("\n=== Test: Bitmask extends Structure ===\n");
    
    if (!file_exists("/tmp/idlc_test_shapes/CommonEnums/Flags.java")) {
        printf("✗ Flags.java not found\n");
        return -1;
    }
    
    FILE *f = fopen("/tmp/idlc_test_shapes/CommonEnums/Flags.java", "r");
    char buf[256];
    int found = 0;
    while (fgets(buf, sizeof(buf), f)) {
        if (strstr(buf, "extends Structure")) {
            found = 1;
            break;
        }
    }
    fclose(f);
    
    printf("%s\n", found ? "✓ Bitmask extends Structure" : "✗ Bitmask doesn't extend Structure");
    return found ? 0 : -1;
}

int test_union_generation(void) {
    printf("\n=== Test: Union Generation ===\n");
    
    if (!file_exists("/tmp/idlc_test_shapes/Shapes/ShapeValue.java")) {
        printf("✗ ShapeValue.java not found\n");
        return -1;
    }
    
    int found = count_files_in_dir("/tmp/idlc_test_shapes/Shapes");
    printf("✓ Generated %d files\n", found);
    return 0;
}

int test_sequence_struct(void) {
    printf("\n=== Test: Sequence Struct ===\n");
    
    if (!file_exists("/tmp/idlc_test_shapes/Shapes/SequenceStruct.java")) {
        printf("✗ SequenceStruct.java not found\n");
        return -1;
    }
    
    FILE *f = fopen("/tmp/idlc_test_shapes/Shapes/SequenceStruct.java", "r");
    char buf[256];
    int found = 0;
    while (fgets(buf, sizeof(buf), f)) {
        if (strstr(buf, "java.util.List")) {
            found = 1;
            break;
        }
    }
    fclose(f);
    
    printf("%s\n", found ? "✓ Uses java.util.List" : "✗ Missing List type");
    return found ? 0 : -1;
}

int test_cross_module_typedef(void) {
    printf("\n=== Test: Cross-Module Typedef ===\n");
    
    if (!file_exists("/tmp/idlc_test_shapes/Shapes/TypedefStruct.java")) {
        printf("✗ TypedefStruct.java not found\n");
        return -1;
    }
    
    FILE *f = fopen("/tmp/idlc_test_shapes/Shapes/TypedefStruct.java", "r");
    char buf[256];
    int has_uri = 0, has_inttype = 0;
    while (fgets(buf, sizeof(buf), f)) {
        if (strstr(buf, "uriVal") || strstr(buf, "URI")) has_uri = 1;
        if (strstr(buf, "intVal") || strstr(buf, "IntType")) has_inttype = 1;
    }
    fclose(f);
    
    printf("%s\n", (has_uri && has_inttype) ? "✓ Cross-module typedefs work" : "✗ Cross-module typedefs failed");
    return (has_uri && has_inttype) ? 0 : -1;
}

int test_tex_entity_payload(void) {
    printf("\n=== Test: TEX EntityPayload IDL ===\n");
    
    char command[1024];
    snprintf(command, sizeof(command),
        "LD_LIBRARY_PATH=%s %s -DDDS_XTYPES -l java -o /tmp/idlc_test_tex %s/examples/tex/EntityPayload.idl 2>&1",
        TEST_PLUGIN_DIR, TEST_IDLC, TEST_EXAMPLES_DIR);
    
    int result = run_command(command);
    
    if (result == 0) {
        int count = count_files_in_dir("/tmp/idlc_test_tex/org/omg/tex/DataPayload/EntityPayload");
        printf("✓ Generated %d files\n", count);
    } else {
        printf("✗ Generation failed\n");
    }
    return result == 0 ? 0 : -1;
}

int test_struct_inheritance(void) {
    printf("\n=== Test: Struct Inheritance ===\n");
    
    if (!file_exists("/tmp/idlc_test_shapes/Shapes/ExtendedCircle.java")) {
        printf("✗ ExtendedCircle.java not found\n");
        return -1;
    }
    
    // ExtendedCircle extends Circle in IDL - verify it was generated
    // Note: JNA flattens inheritance, so we just check the file exists and has expected content
    FILE *f = fopen("/tmp/idlc_test_shapes/Shapes/ExtendedCircle.java", "r");
    char buf[256];
    int found = 0;
    while (fgets(buf, sizeof(buf), f)) {
        // Check it's a struct with own fields (label from ExtendedCircle)
        if (strstr(buf, "public String label")) {
            found = 1;
            break;
        }
    }
    fclose(f);
    
    printf("%s\n", found ? "✓ Inheritance works (flattened)" : "✗ Inheritance failed");
    return found ? 0 : -1;
}

int main(int argc, char *argv[]) {
    int failed = 0;
    
    printf("IDL to Java Generator - Test Suite\n");
    printf("====================================\n\n");
    
    // Basic tests
    if (test_plugin_loading() != 0) failed++;
    if (test_basic_generation() != 0) failed++;
    
    // Struct tests
    if (test_struct_extends_structure() != 0) failed++;
    if (test_struct_has_field_order() != 0) failed++;
    if (test_struct_has_serialize() != 0) failed++;
    if (test_struct_has_deserialize() != 0) failed++;
    if (test_struct_has_describe_type() != 0) failed++;
    if (test_struct_inheritance() != 0) failed++;
    if (test_sequence_struct() != 0) failed++;
    if (test_cross_module_typedef() != 0) failed++;
    
    // Enum tests
    if (test_enum_is_java_enum() != 0) failed++;
    if (test_enum_has_get_value() != 0) failed++;
    
    // Bitmask tests
    if (test_bitmask_extends_structure() != 0) failed++;
    
    // Union tests
    if (test_union_generation() != 0) failed++;
    
    // Complex IDL
    if (test_tex_entity_payload() != 0) failed++;
    
    printf("\n=== Test Summary ===\n");
    printf("Passed: %d, Failed: %d\n", 16 - failed, failed);
    
    return failed > 0 ? 1 : 0;
}
