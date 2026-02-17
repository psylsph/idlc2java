/*
 * Copyright (c) 2024 IDLC Java Generator Contributors
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License 1.0
 * which is available at http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define TEST_IDLC "/opt/cyclonedds/bin/idlc"
#define TEST_PLUGIN_DIR "/home/stuart/repos/idlc2java/build"
#define TEST_EXAMPLES_DIR "/home/stuart/repos/idlc2java/examples"
#define TEST_OUTPUT_DIR "/tmp/idlc_java_test"

int run_command(const char *command, const char *output_file) {
    fprintf(stderr, "DEBUG: Running command: %s\n", command);
    FILE *fp = popen(command, "r");
    if (!fp) {
        fprintf(stderr, "Failed to run command: %s\n", command);
        return -1;
    }
    
    // Read all output to avoid pipe issues
    char buffer[4096];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer)-1, fp)) > 0) {
        buffer[n] = '\0';
        fprintf(stderr, "OUTPUT: %s", buffer);
    }
    
    int status = pclose(fp);
    fprintf(stderr, "DEBUG: Command exited with status: %d (WIFEXITED=%d, WIFSIGNALED=%d)\n", 
            status, WIFEXITED(status), WIFSIGNALED(status));
    if (WIFSIGNALED(status)) {
        fprintf(stderr, "DEBUG: Signal number: %d\n", WTERMSIG(status));
    }
    return WEXITSTATUS(status);
}

int test_plugin_loading(void) {
    printf("=== Test: Plugin Loading ===\n");
    
    char command[512];
    snprintf(command, sizeof(command), 
        "LD_LIBRARY_PATH=%s %s -l java -o /tmp/test_plugin_check %s/shapes.idl 2>&1",
        TEST_PLUGIN_DIR,
        TEST_IDLC,
        TEST_EXAMPLES_DIR);
    
    int result = run_command(command, NULL);
    
    if (result == 0) {
        printf("✓ Plugin loaded successfully\n");
    } else {
        printf("✗ Plugin loading failed (exit code: %d)\n", result);
    }
    
    return result == 0 ? 0 : -1;
}

int test_basic_generation(void) {
    printf("\n=== Test: Basic Generation ===\n");
    
    char command[1024];
    snprintf(command, sizeof(command),
        "LD_LIBRARY_PATH=%s %s -l java -o %s %s/shapes.idl 2>&1",
        TEST_PLUGIN_DIR,
        TEST_IDLC,
        TEST_OUTPUT_DIR,
        TEST_EXAMPLES_DIR);
    
    int result = run_command(command, NULL);
    
    if (result == 0) {
        printf("✓ Code generation succeeded\n");
        
        char check_cmd[512];
        snprintf(check_cmd, sizeof(check_cmd),
            "ls -R %s 2>&1 | head -20",
            TEST_OUTPUT_DIR);
        system(check_cmd);
    } else {
        printf("✗ Code generation failed (exit code: %d)\n", result);
    }
    
    return result == 0 ? 0 : -1;
}

int main(int argc, char *argv[]) {
    int failed = 0;
    
    printf("IDL to Java Generator - Test Suite\n");
    printf("====================================\n\n");
    
    if (test_plugin_loading() != 0) failed++;
    if (test_basic_generation() != 0) failed++;
    
    printf("\n=== Test Summary ===\n");
    if (failed == 0) {
        printf("All tests passed!\n");
        return 0;
    } else {
        printf("%d test(s) failed\n", failed);
        return 1;
    }
}
