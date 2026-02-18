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

#define TEST_IDLC "/opt/cyclonedds/bin/idlc"
#define TEST_PLUGIN_DIR "/home/stuart/repos/idlc2java/build"
#define TEST_EXAMPLES_DIR "/home/stuart/repos/idlc2java/examples"

int run_command(const char *command) {
    fprintf(stderr, "DEBUG: Running command: %s\n", command);
    FILE *fp = popen(command, "r");
    if (!fp) {
        fprintf(stderr, "Failed to run command: %s\n", command);
        return -1;
    }
    
    char buffer[4096];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer)-1, fp)) > 0) {
        buffer[n] = '\0';
        fprintf(stderr, "OUTPUT: %s", buffer);
    }
    
    int status = pclose(fp);
    int exit_code = WEXITSTATUS(status);
    fprintf(stderr, "DEBUG: Command exited with status: %d (WIFEXITED=%d, WIFSIGNALED=%d)\n", 
            exit_code, WIFEXITED(status), WIFSIGNALED(status));
    if (WIFSIGNALED(status)) {
        fprintf(stderr, "DEBUG: Signal number: %d\n", WTERMSIG(status));
    }
    return exit_code;
}

int test_plugin_loading(void) {
    printf("=== Test: Plugin Loading ===\n");
    
    char command[512];
    snprintf(command, sizeof(command), 
        "LD_LIBRARY_PATH=%s %s -l java -o /tmp/test_plugin_check -I %s/all-types %s/all-types/shapes.idl 2>&1",
        TEST_PLUGIN_DIR,
        TEST_IDLC,
        TEST_EXAMPLES_DIR,
        TEST_EXAMPLES_DIR);
    
    int result = run_command(command);
    
    if (result == 0) {
        printf("✓ Plugin loaded successfully\n");
    } else {
        printf("✗ Plugin loading failed (exit code: %d)\n", result);
    }
    
    return result == 0 ? 0 : -1;
}

int test_basic_generation(void) {
    printf("\n=== Test: Basic Generation (shapes.idl) ===\n");
    
    char command[1024];
    snprintf(command, sizeof(command),
        "LD_LIBRARY_PATH=%s %s -l java -o /tmp/idlc_java_test -I %s/all-types %s/all-types/shapes.idl 2>&1",
        TEST_PLUGIN_DIR,
        TEST_IDLC,
        TEST_EXAMPLES_DIR,
        TEST_EXAMPLES_DIR);
    
    int result = run_command(command);
    
    if (result == 0) {
        printf("✓ Code generation succeeded\n");
    } else {
        printf("✗ Code generation failed (exit code: %d)\n", result);
    }
    
    return result == 0 ? 0 : -1;
}

int test_common_types(void) {
    printf("\n=== Test: Common Types Generation ===\n");
    
    char command[1024];
    snprintf(command, sizeof(command),
        "LD_LIBRARY_PATH=%s %s -DDDS_XTYPES -l java -o /tmp/test_common_types -I %s/all-types %s/all-types/common_types.idl 2>&1",
        TEST_PLUGIN_DIR,
        TEST_IDLC,
        TEST_EXAMPLES_DIR,
        TEST_EXAMPLES_DIR);
    
    int result = run_command(command);
    
    if (result == 0) {
        printf("✓ Common types generation succeeded\n");
    } else {
        printf("✗ Common types generation failed (exit code: %d)\n", result);
    }
    
    return result == 0 ? 0 : -1;
}

int test_util_idl(void) {
    printf("\n=== Test: Util IDL Generation ===\n");
    
    char command[1024];
    snprintf(command, sizeof(command),
        "LD_LIBRARY_PATH=%s %s -DDDS_XTYPES -l java -o /tmp/test_util_idl %s/tex/Util.idl 2>&1",
        TEST_PLUGIN_DIR,
        TEST_IDLC,
        TEST_EXAMPLES_DIR);
    
    int result = run_command(command);
    
    if (result == 0) {
        printf("✓ Util IDL generation succeeded\n");
    } else {
        printf("✗ Util IDL generation failed (exit code: %d)\n", result);
    }
    
    return result == 0 ? 0 : -1;
}

int test_struct_with_primitives(void) {
    printf("\n=== Test: Struct with All Primitives ===\n");
    
    char command[1024];
    snprintf(command, sizeof(command),
        "LD_LIBRARY_PATH=%s %s -l java -o /tmp/test_primitives -I %s/all-types %s/all-types/shapes.idl 2>&1 | grep -c 'PrimitiveStruct'",
        TEST_PLUGIN_DIR,
        TEST_IDLC,
        TEST_EXAMPLES_DIR,
        TEST_EXAMPLES_DIR);
    
    int result = run_command(command);
    
    if (result == 0) {
        printf("✓ Primitive struct generation succeeded\n");
    } else {
        printf("✗ Primitive struct generation failed (exit code: %d)\n", result);
    }
    
    return result == 0 ? 0 : -1;
}

int test_union_generation(void) {
    printf("\n=== Test: Union Generation ===\n");
    
    char command[1024];
    snprintf(command, sizeof(command),
        "LD_LIBRARY_PATH=%s %s -l java -o /tmp/test_union -I %s/all-types %s/all-types/shapes.idl 2>&1 | grep -c 'ShapeValue'",
        TEST_PLUGIN_DIR,
        TEST_IDLC,
        TEST_EXAMPLES_DIR,
        TEST_EXAMPLES_DIR);
    
    int result = run_command(command);
    
    if (result == 0) {
        printf("✓ Union generation succeeded\n");
    } else {
        printf("✗ Union generation failed (exit code: %d)\n", result);
    }
    
    return result == 0 ? 0 : -1;
}

int test_enum_generation(void) {
    printf("\n=== Test: Enum Generation ===\n");
    
    char command[1024];
    snprintf(command, sizeof(command),
        "LD_LIBRARY_PATH=%s %s -l java -o /tmp/test_enum -I %s/all-types %s/all-types/shapes.idl 2>&1 | grep -c 'ShapeType'",
        TEST_PLUGIN_DIR,
        TEST_IDLC,
        TEST_EXAMPLES_DIR,
        TEST_EXAMPLES_DIR);
    
    int result = run_command(command);
    
    if (result == 0) {
        printf("✓ Enum generation succeeded\n");
    } else {
        printf("✗ Enum generation failed (exit code: %d)\n", result);
    }
    
    return result == 0 ? 0 : -1;
}

int test_bitmask_generation(void) {
    printf("\n=== Test: Bitmask Generation ===\n");
    
    char command[1024];
    snprintf(command, sizeof(command),
        "LD_LIBRARY_PATH=%s %s -l java -o /tmp/test_bitmask -I %s/all-types %s/all-types/shapes.idl 2>&1 | grep -c 'Flags'",
        TEST_PLUGIN_DIR,
        TEST_IDLC,
        TEST_EXAMPLES_DIR,
        TEST_EXAMPLES_DIR);
    
    int result = run_command(command);
    
    if (result == 0) {
        printf("✓ Bitmask generation succeeded\n");
    } else {
        printf("✗ Bitmask generation failed (exit code: %d)\n", result);
    }
    
    return result == 0 ? 0 : -1;
}

int main(int argc, char *argv[]) {
    int failed = 0;
    
    printf("IDL to Java Generator - Test Suite\n");
    printf("====================================\n\n");
    
    if (test_plugin_loading() != 0) failed++;
    if (test_basic_generation() != 0) failed++;
    if (test_common_types() != 0) failed++;
    if (test_util_idl() != 0) failed++;
    if (test_struct_with_primitives() != 0) failed++;
    if (test_union_generation() != 0) failed++;
    if (test_enum_generation() != 0) failed++;
    if (test_bitmask_generation() != 0) failed++;
    
    printf("\n=== Test Summary ===\n");
    if (failed == 0) {
        printf("All tests passed!\n");
        return 0;
    } else {
        printf("%d test(s) failed\n", failed);
        return 1;
    }
}
