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
#include <errno.h>
#include <ctype.h>
#include "idlc_java.h"

typedef struct string_builder string_builder_t;

extern string_builder_t *sb_create(void);
extern void sb_destroy(string_builder_t *sb);
extern int sb_append(string_builder_t *sb, const char *str);
extern int sb_appendf(string_builder_t *sb, const char *format, ...);
extern const char *sb_string(const string_builder_t *sb);

extern char *resolve_package(const idl_node_t *node, const char *prefix);
extern char *java_type_name(const idl_type_spec_t *type_spec, bool boxed);

static int generate_record_header(string_builder_t *sb, const char *package, const char *class_name);
static int generate_record_members(string_builder_t *sb, const idl_struct_t *struct_def);
static int generate_record_methods(string_builder_t *sb, const idl_struct_t *struct_def, const char *class_name);

static int generate_record_header(string_builder_t *sb, const char *package, const char *class_name) {
    sb_appendf(sb, "package %s;\n\n", package);
    sb_append(sb, "import java.io.Serializable;\n\n");
    
    sb_appendf(sb, "public record %s(", class_name);
    
    return 0;
}

static int generate_record_members(string_builder_t *sb, const idl_struct_t *struct_def) {
    if (!struct_def->members) {
        sb_append(sb, ") implements Serializable {\n\n");
        return 0;
    }
    
    int member_count = 0;
    
    for (const idl_member_t *member = struct_def->members; member; member_count++) {
        if (member_count > 0) {
            sb_append(sb, ", ");
        }
        
        /* Get member name from declarators */
        const char *member_name = "field";
        if (member->declarators && member->declarators->name && member->declarators->name->identifier) {
            member_name = member->declarators->name->identifier;
        }
        
        /* Get Java type */
        char *java_type = java_type_name(member->type_spec, false);
        sb_appendf(sb, "%s %s", java_type, member_name);
        free(java_type);
        
        /* Try to get next member */
        idl_node_t *next_node = (idl_node_t *)member;
        if (!next_node->next) break;
        member = (const idl_member_t *)next_node->next;
    }
    
    sb_append(sb, ") implements Serializable {\n\n");
    
    return member_count;
}

static int generate_record_methods(string_builder_t *sb, const idl_struct_t *struct_def, const char *class_name) {
    sb_append(sb, "    @Override\n");
    sb_append(sb, "    public String toString() {\n");
    sb_appendf(sb, "        return \"%s[\" +\n", class_name);
    
    /* Generate field list */
    int field_count = 0;
    for (const idl_member_t *member = struct_def->members; member; field_count++) {
        const char *member_name = "field";
        if (member->declarators && member->declarators->name && member->declarators->name->identifier) {
            member_name = member->declarators->name->identifier;
        }
        
        if (field_count > 0) {
            sb_append(sb, " +\n");
            sb_append(sb, "            \", \" + \"");
            sb_appendf(sb, "%s=\" + %s()", member_name, member_name);
        } else {
            sb_append(sb, "            \"");
            sb_appendf(sb, "%s=\" + %s()", member_name, member_name);
        }
        
        /* Get next member */
        idl_node_t *next_node = (idl_node_t *)member;
        if (!next_node->next) break;
        member = (const idl_member_t *)next_node->next;
    }
    
    sb_append(sb, " +\n            \"]\";\n");
    sb_append(sb, "    }\n\n");
    
    return 0;
}

int generate_java_record(const idl_struct_t *struct_def, const char *output_dir, const char *prefix, bool disable_cdr, const char *class_name) {
    if (!struct_def || !output_dir) return -1;
    
    const char *actual_class_name = class_name ? class_name : "GeneratedStruct";
    
    char *package = resolve_package((const idl_node_t *)struct_def, prefix);
    
    string_builder_t *sb = sb_create();
    if (!sb) {
        free(package);
        return -1;
    }
    
    generate_record_header(sb, package, actual_class_name);
    generate_record_members(sb, struct_def);
    generate_record_methods(sb, struct_def, actual_class_name);
    
    sb_append(sb, "}\n");
    
    /* Create package directory */
    char package_path[512];
    if (strcmp(output_dir, ".") == 0) {
        snprintf(package_path, sizeof(package_path), "%s", package);
    } else {
        snprintf(package_path, sizeof(package_path), "%s/%s", output_dir, package);
    }
    for (char *p = package_path; *p; p++) {
        if (*p == '.') {
            *p = '\0';
            mkdir(package_path, 0755);
            *p = '/';
        }
    }
    
    mkdir(package_path, 0755);
    
    /* Write file */
    char file_path[768];
    snprintf(file_path, sizeof(file_path), "%s/%s.java", package_path, actual_class_name);
    
    FILE *f = fopen(file_path, "w");
    if (f) {
        fputs(sb_string(sb), f);
        fclose(f);
        printf("  Created: %s\n", file_path);
    } else {
        fprintf(stderr, "  Error: Could not create file: %s\n", file_path);
        free(package);
        sb_destroy(sb);
        return -1;
    }
    
    free(package);
    sb_destroy(sb);
    
    return 0;
}

int generate_java_enum(const idl_enum_t *enum_def, const char *output_dir, const char *prefix, const char *enum_name) {
    if (!enum_def || !output_dir) return -1;
    
    char *package = resolve_package((const idl_node_t *)enum_def, prefix);
    const char *actual_enum_name = enum_name ? enum_name : "GeneratedEnum";
    
    string_builder_t *sb = sb_create();
    if (!sb) {
        free(package);
        return -1;
    }
    
    sb_appendf(sb, "package %s;\n\n", package);
    
    sb_appendf(sb, "public enum %s {\n", actual_enum_name);
    
    /* Generate enum values from the enumerators list */
    int enum_count = 0;
    for (const idl_enumerator_t *enumerator = enum_def->enumerators; enumerator; enumerator = (const idl_enumerator_t *)((idl_node_t *)enumerator)->next) {
        if (enum_count > 0) {
            sb_append(sb, ",\n");
        }
        const char *enum_value = "VALUE";
        if (enumerator->name && enumerator->name->identifier) {
            enum_value = enumerator->name->identifier;
        }
        sb_appendf(sb, "    %s(%d)", enum_value, enum_count);
        enum_count++;
    }
    sb_append(sb, ";\n\n");
    
    sb_append(sb, "    private final int value;\n\n");
    sb_appendf(sb, "    %s(int value) {\n", actual_enum_name);
    sb_append(sb, "        this.value = value;\n");
    sb_append(sb, "    }\n\n");
    sb_append(sb, "    public int getValue() {\n");
    sb_append(sb, "        return value;\n");
    sb_append(sb, "    }\n");
    sb_append(sb, "}\n");
    
    /* Create package directory */
    char package_path[512];
    if (strcmp(output_dir, ".") == 0) {
        snprintf(package_path, sizeof(package_path), "%s", package);
    } else {
        snprintf(package_path, sizeof(package_path), "%s/%s", output_dir, package);
    }
    for (char *p = package_path; *p; p++) {
        if (*p == '.') {
            *p = '\0';
            mkdir(package_path, 0755);
            *p = '/';
        }
    }
    
    mkdir(package_path, 0755);
    
    /* Write file */
    char file_path[768];
    snprintf(file_path, sizeof(file_path), "%s/%s.java", package_path, actual_enum_name);
    
    FILE *f = fopen(file_path, "w");
    if (f) {
        fputs(sb_string(sb), f);
        fclose(f);
        printf("  Created: %s\n", file_path);
    } else {
        fprintf(stderr, "  Error: Could not create file: %s\n", file_path);
        free(package);
        sb_destroy(sb);
        return -1;
    }
    
    free(package);
    sb_destroy(sb);
    
    return 0;
}

int generate_java_typedef(const idl_typedef_t *typedef_def, const char *output_dir, const char *prefix) {
    if (!typedef_def || !output_dir) return -1;
    if (!typedef_def->declarators || !typedef_def->declarators->name || !typedef_def->declarators->name->identifier) {
        return 0;
    }
    
    const char *typedef_name = typedef_def->declarators->name->identifier;
    char *java_type = java_type_name(typedef_def->type_spec, false);
    
    char *package = resolve_package((const idl_node_t *)typedef_def, prefix);
    
    string_builder_t *sb = sb_create();
    if (!sb) {
        free(java_type);
        free(package);
        return -1;
    }
    
    sb_appendf(sb, "package %s;\n\n", package);
    sb_appendf(sb, "public class %s {\n\n", typedef_name);
    sb_appendf(sb, "    private %s value;\n\n", java_type);
    sb_appendf(sb, "    public %s() {\n", typedef_name);
    sb_append(sb, "    }\n\n");
    sb_appendf(sb, "    public %s(%s value) {\n", typedef_name, java_type);
    sb_append(sb, "        this.value = value;\n");
    sb_append(sb, "    }\n\n");
    sb_appendf(sb, "    public %s getValue() {\n", java_type);
    sb_append(sb, "        return value;\n");
    sb_append(sb, "    }\n\n");
    sb_appendf(sb, "    public void setValue(%s value) {\n", java_type);
    sb_append(sb, "        this.value = value;\n");
    sb_append(sb, "    }\n\n");
    sb_append(sb, "}\n");
    
    char package_path[512];
    if (strcmp(output_dir, ".") == 0) {
        snprintf(package_path, sizeof(package_path), "%s", package);
    } else {
        snprintf(package_path, sizeof(package_path), "%s/%s", output_dir, package);
    }
    for (char *p = package_path; *p; p++) {
        if (*p == '.') {
            *p = '\0';
            mkdir(package_path, 0755);
            *p = '/';
        }
    }
    
    mkdir(package_path, 0755);
    
    char file_path[768];
    snprintf(file_path, sizeof(file_path), "%s/%s.java", package_path, typedef_name);
    
    FILE *f = fopen(file_path, "w");
    if (f) {
        fputs(sb_string(sb), f);
        fclose(f);
        printf("  Created: %s\n", file_path);
    } else {
        fprintf(stderr, "  Error: Could not create file: %s\n", file_path);
        free(java_type);
        free(package);
        sb_destroy(sb);
        return -1;
    }
    
    free(java_type);
    free(package);
    sb_destroy(sb);
    
    return 0;
}
