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
#include <stdbool.h>
#include <sys/stat.h>
#include "idlc/generator.h"
#include "idlc/options.h"
#include "idl/processor.h"
#include "idl/tree.h"
#include "idlc_java.h"

typedef struct string_builder string_builder_t;

extern string_builder_t *sb_create(void);
extern void sb_destroy(string_builder_t *sb);
extern int sb_append(string_builder_t *sb, const char *str);
extern int sb_appendf(string_builder_t *sb, const char *format, ...);
extern const char *sb_string(const string_builder_t *sb);

extern int generate_java_record(const idl_struct_t *struct_def, const char *output_dir, const char *prefix, bool disable_cdr, const char *class_name);
extern int generate_java_enum(const idl_enum_t *enum_def, const char *output_dir, const char *prefix, const char *enum_name);
extern int generate_java_typedef(const idl_typedef_t *typedef_def, const char *output_dir, const char *prefix);
extern char *resolve_package(const idl_node_t *node, const char *prefix);

typedef struct generator_state {
    const char *output_dir;
    const char *package_prefix;
    bool use_arrays_for_sequences;
    bool disable_cdr;
    bool generate_records;
    int errors;
    int struct_count;
    int enum_count;
    bool in_module;
    char current_module_name[64];
} generator_state_t;

static const char *java_package_prefix = NULL;
static int java_use_arrays_flag = 0;
static int java_disable_cdr_flag = 0;

static void process_node(idl_node_t *node, generator_state_t *state) {
    if (!node) return;
    
    idl_mask_t mask = idl_mask(node);
    
    if (mask & IDL_STRUCT) {
        idl_struct_t *struct_def = (idl_struct_t *)node;
        state->struct_count++;
        
        const char *struct_name = "GeneratedStruct";
        if (struct_def->name && struct_def->name->identifier) {
            struct_name = struct_def->name->identifier;
        }
        
        printf("Found struct: %s\n", struct_name);
        
        if (generate_java_record(struct_def, state->output_dir, state->package_prefix, state->disable_cdr, struct_name) != 0) {
            fprintf(stderr, "Error generating struct: %s\n", struct_name);
            state->errors++;
        }
    }
    else if (mask & IDL_ENUM) {
        idl_enum_t *enum_def = (idl_enum_t *)node;
        
        state->enum_count++;
        
        const char *enum_name = "GeneratedEnum";
        if (enum_def->name && enum_def->name->identifier) {
            enum_name = enum_def->name->identifier;
        }
        
        printf("Found enum: %s\n", enum_name);
        
        if (generate_java_enum(enum_def, state->output_dir, state->package_prefix, enum_name) != 0) {
            fprintf(stderr, "Error generating enum: %s\n", enum_name);
            state->errors++;
        }
    }
    else if (mask & IDL_TYPEDEF) {
        idl_typedef_t *typedef_def = (idl_typedef_t *)node;
        
        printf("Found typedef\n");
        
        if (generate_java_typedef(typedef_def, state->output_dir, state->package_prefix) != 0) {
            fprintf(stderr, "Error generating typedef\n");
            state->errors++;
        }
    }
    else if (mask & IDL_MODULE) {
        idl_module_t *module = (idl_module_t *)node;
        
        const char *module_name = "module";
        if (module->name && module->name->identifier) {
            module_name = module->name->identifier;
        }
        
        strncpy(state->current_module_name, module_name, sizeof(state->current_module_name) - 1);
        state->current_module_name[sizeof(state->current_module_name) - 1] = '\0';
        
        printf("Processing module: %s\n", module_name);
        
        if (module->definitions) {
            state->in_module = true;
            
            idl_node_t *def = (idl_node_t *)module->definitions;
            while (def) {
                process_node(def, state);
                def = def->next;
            }
            
            state->in_module = false;
        }
    }
    
    if (node->next && !state->in_module) {
        process_node(node->next, state);
    }
}

static int generate_types(const idl_pstate_t *pstate, generator_state_t *state) {
    if (!pstate || !pstate->root) {
        fprintf(stderr, "Error: No AST to process\n");
        return -1;
    }
    
    process_node((idl_node_t *)pstate->root, state);
    
    printf("Found %d structs and %d enums\n", state->struct_count, state->enum_count);
    
    return state->errors;
}

static const idlc_option_t **generator_options(void) {
    static idlc_option_t options[] = {
        {
            .type = IDLC_STRING,
            .store = { .string = &java_package_prefix },
            .option = 0,
            .suboption = "java-package-prefix",
            .argument = "<prefix>",
            .help = "Prefix for generated Java packages"
        },
        {
            .type = IDLC_FLAG,
            .store = { .flag = &java_use_arrays_flag },
            .option = 0,
            .suboption = "java-use-arrays",
            .argument = NULL,
            .help = "Use arrays instead of List for sequences"
        },
        {
            .type = IDLC_FLAG,
            .store = { .flag = &java_disable_cdr_flag },
            .option = 0,
            .suboption = "java-disable-cdr",
            .argument = NULL,
            .help = "Disable CDR serialization code generation"
        },
        { .type = 0 }
    };
    
    static const idlc_option_t *option_ptrs[] = {
        &options[0],
        &options[1],
        &options[2],
        NULL
    };
    
    return option_ptrs;
}

int generate(const idl_pstate_t *pstate, const idlc_generator_config_t *config) {
    if (!pstate || !pstate->root) {
        fprintf(stderr, "Error: No parsed IDL available\n");
        return -1;
    }
    
    const char *output_dir = config && config->output_dir ? config->output_dir : ".";
    if (output_dir[0] == '\0') {
        output_dir = ".";
    }
    
    generator_state_t state = {
        .output_dir = output_dir,
        .package_prefix = java_package_prefix,
        .use_arrays_for_sequences = (java_use_arrays_flag != 0),
        .disable_cdr = (java_disable_cdr_flag != 0),
        .generate_records = true,
        .errors = 0,
        .struct_count = 0,
        .enum_count = 0,
        .in_module = false,
        .current_module_name = ""
    };
    
    mkdir(state.output_dir, 0755);
    
    printf("Generating Java code to: %s\n", state.output_dir);
    
    int ret = generate_types(pstate, &state);
    
    if (ret == 0) {
        printf("Java code generation completed successfully\n");
    } else {
        fprintf(stderr, "Java code generation completed with %d errors\n", state.errors);
    }
    
    return ret;
}
