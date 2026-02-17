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
#include <ctype.h>
#include "idlc_java.h"

typedef struct string_builder string_builder_t;

extern string_builder_t *sb_create(void);
extern void sb_destroy(string_builder_t *sb);
extern int sb_append(string_builder_t *sb, const char *str);
extern int sb_appendf(string_builder_t *sb, const char *format, ...);
extern const char *sb_string(const string_builder_t *sb);

extern char *resolve_package(const idl_node_t *node, const char *prefix);

char *java_type_name(const idl_type_spec_t *type_spec, bool boxed) {
    if (!type_spec) return strdup("Object");

    idl_type_t type = idl_type(type_spec);

    // Handle typedef by returning the typedef name
    if (type == IDL_TYPEDEF) {
        // For typedefs from other modules, use idl_name() which works across module boundaries
        const idl_name_t *type_name = idl_name(type_spec);
        if (type_name && type_name->identifier) {
            return strdup(type_name->identifier);
        }
        return strdup("Object");
    }

    switch (type) {
        case IDL_BOOL:
            return strdup(boxed ? "Boolean" : "boolean");
        case IDL_OCTET:
        case IDL_CHAR:
            return strdup(boxed ? "Byte" : "byte");
        case IDL_SHORT:
            return strdup(boxed ? "Short" : "short");
        case IDL_USHORT:
            return strdup(boxed ? "Character" : "char");
        case IDL_LONG:
            return strdup(boxed ? "Integer" : "int");
        case IDL_ULONG:
            return strdup(boxed ? "Integer" : "int");
        case IDL_LLONG:
            return strdup(boxed ? "Long" : "long");
        case IDL_ULLONG:
            return strdup(boxed ? "Long" : "long");
        case IDL_FLOAT:
            return strdup(boxed ? "Float" : "float");
        case IDL_DOUBLE:
            return strdup(boxed ? "Double" : "double");
        case IDL_STRING:
            return strdup("String");
        case IDL_WSTRING:
            return strdup("String");
        case IDL_SEQUENCE: {
            const idl_sequence_t *seq = (const idl_sequence_t *)type_spec;
            char *element_type = java_type_name(seq->type_spec, true);
            char *result = malloc(strlen(element_type) + 20);
            sprintf(result, "java.util.List<%s>", element_type);
            free(element_type);
            return result;
        }
        case IDL_STRUCT:
        case IDL_UNION:
        case IDL_ENUM:
        case IDL_BITMASK: {
            const idl_name_t *name = NULL;
            idl_mask_t type_mask = idl_mask(type_spec);
            if (type_mask & IDL_STRUCT) {
                name = ((const idl_struct_t *)type_spec)->name;
            } else if (type_mask & IDL_UNION) {
                name = ((const idl_union_t *)type_spec)->name;
            } else if (type_mask & IDL_ENUM) {
                name = ((const idl_enum_t *)type_spec)->name;
            } else if (type_mask & IDL_BITMASK) {
                name = ((const idl_bitmask_t *)type_spec)->name;
            }
            if (name && name->identifier) {
                return strdup(name->identifier);
            }
            return strdup("Object");
        }
        case IDL_TYPEDEF: {
            const idl_typedef_t *td = (const idl_typedef_t *)type_spec;
            if (td && td->declarators && td->declarators->name && td->declarators->name->identifier) {
                return strdup(td->declarators->name->identifier);
            }
            return strdup("Object");
        }
        default:
            return strdup("Object");
    }
}

const char *java_default_value(idl_type_t type) {
    switch (type) {
        case IDL_BOOL: return "false";
        case IDL_CHAR:
        case IDL_OCTET:
        case IDL_SHORT:
        case IDL_USHORT:
        case IDL_LONG:
        case IDL_ULONG:
        case IDL_INT8:
        case IDL_UINT8:
        case IDL_INT16:
        case IDL_UINT16:
        case IDL_INT32:
        case IDL_UINT32:
        case IDL_INT64:
        case IDL_UINT64:
        case IDL_LLONG:
        case IDL_ULLONG: return "0";
        case IDL_FLOAT:
        case IDL_DOUBLE:
        case IDL_LDOUBLE: return "0.0";
        default: return "null";
    }
}
