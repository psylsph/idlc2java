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
#include "idlc/generator.h"
#include "idl/processor.h"
#include "idl/tree.h"
#include "idl/visit.h"
#include "idlc_java.h"

typedef struct string_builder string_builder_t;

extern string_builder_t *sb_create(void);
extern void sb_destroy(string_builder_t *sb);
extern int sb_append(string_builder_t *sb, const char *str);
extern int sb_appendf(string_builder_t *sb, const char *format, ...);
extern const char *sb_string(const string_builder_t *sb);

extern char *resolve_package(const idl_node_t *node, const char *prefix);
extern char *java_type_name(const idl_type_spec_t *type_spec, bool boxed);
extern const char *java_default_value(idl_type_t type);

int generate_types_for_module(const idl_module_t *module, const char *output_dir, const char *prefix) {
    if (!module || !output_dir) return -1;
    
    int count = 0;
    
    for (idl_node_t *def = (idl_node_t *)module->definitions; def; def = def->next) {
        idl_mask_t mask = idl_mask(def);
        
        if (mask & IDL_STRUCT) {
            count++;
        } else if (mask & IDL_ENUM) {
            count++;
        } else if (mask & IDL_UNION) {
            // TODO: Generate union
        } else if (mask & IDL_BITMASK) {
            // TODO: Generate bitmask
        } else if (mask & IDL_MODULE) {
            count += generate_types_for_module((const idl_module_t *)def, output_dir, prefix);
        }
    }
    
    return count;
}

int validate_idl(const idl_pstate_t *pstate) {
    if (!pstate || !pstate->root) {
        return -1;
    }
    
    int warnings = 0;
    
    return warnings;
}

char *get_type_description(const idl_type_spec_t *type_spec) {
    if (!type_spec) return strdup("unknown");
    
    idl_mask_t mask = idl_mask(type_spec);
    
    if (mask & IDL_BASE_TYPE) {
        return strdup("primitive");
    } else if (mask & IDL_STRING) {
        return strdup("string");
    } else if (mask & IDL_SEQUENCE) {
        return strdup("sequence");
    } else if (mask & IDL_STRUCT) {
        const idl_struct_t *s = (const idl_struct_t *)type_spec;
        const char *name = s->name ? idl_identifier(s->name) : "anonymous";
        char *desc = malloc(strlen(name) + 20);
        sprintf(desc, "struct %s", name);
        return desc;
    } else if (mask & IDL_ENUM) {
        const idl_enum_t *e = (const idl_enum_t *)type_spec;
        const char *name = e->name ? idl_identifier(e->name) : "anonymous";
        char *desc = malloc(strlen(name) + 20);
        sprintf(desc, "enum %s", name);
        return desc;
    }
    
    return strdup("complex");
}

void print_statistics(const idl_pstate_t *pstate) {
    if (!pstate) return;
    
    printf("\n--- IDL Statistics ---\n");
    
    int structs = 0, enums = 0, unions = 0, modules = 0, typedefs = 0;
    
    printf("Structures: %d\n", structs);
    printf("Enumerations: %d\n", enums);
    printf("Unions: %d\n", unions);
    printf("Modules: %d\n", modules);
    printf("Type definitions: %d\n", typedefs);
    printf("---------------------\n\n");
}
