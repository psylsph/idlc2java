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

static const char *get_member_name(const idl_member_t *member) {
    if (member->declarators && member->declarators->name && member->declarators->name->identifier) {
        return member->declarators->name->identifier;
    }
    return "field";
}

static idl_type_t resolve_typedef_type(const idl_type_spec_t *type_spec) {
    // Use idl_unalias to resolve typedefs to their base type
    const idl_type_spec_t *unaliased = idl_unalias(type_spec);
    return idl_type(unaliased);
}

static const char *dynamic_type_kind(idl_type_t type) {
    switch (type) {
        case IDL_BOOL: return "BOOLEAN";
        case IDL_OCTET:
        case IDL_CHAR: return "OCTET";
        case IDL_SHORT:
        case IDL_USHORT: return "INT16";
        case IDL_LONG:
        case IDL_ULONG: return "INT32";
        case IDL_LLONG:
        case IDL_ULLONG: return "INT64";
        case IDL_FLOAT: return "FLOAT32";
        case IDL_DOUBLE: return "FLOAT64";
        case IDL_STRING: return "STRING";
        case IDL_WSTRING: return "STRING";
        case IDL_SEQUENCE: return "SEQUENCE";
        case IDL_STRUCT: return "STRUCT";
        case IDL_UNION: return "UNION";
        case IDL_ENUM: return "ENUM";
        case IDL_BITMASK: return "BITMASK";
        default: return "UNKNOWN";
    }
}

static int generate_structure_header(string_builder_t *sb, const char *package, const char *class_name) {
    sb_appendf(sb, "package %s;\n\n", package);
    sb_append(sb, "import com.sun.jna.Structure;\n");
    sb_append(sb, "import java.nio.ByteBuffer;\n");
    sb_append(sb, "import java.nio.ByteOrder;\n");
    sb_append(sb, "import java.nio.charset.StandardCharsets;\n\n");
    sb_appendf(sb, "public class %s extends Structure {\n\n", class_name);
    return 0;
}

static int generate_field_order(string_builder_t *sb, const idl_struct_t *struct_def) {
    sb_append(sb, "    @Structure.FieldOrder({");
    
    int count = 0;
    for (const idl_member_t *member = struct_def->members; member; ) {
        if (count > 0) sb_append(sb, ", ");
        sb_appendf(sb, "\"%s\"", get_member_name(member));
        count++;
        idl_node_t *next_node = (idl_node_t *)member;
        if (!next_node->next) break;
        member = (const idl_member_t *)next_node->next;
    }
    
    sb_append(sb, "})\n\n");
    return 0;
}

static int generate_structure_fields(string_builder_t *sb, const idl_struct_t *struct_def) {
    for (const idl_member_t *member = struct_def->members; member; ) {
        const char *name = get_member_name(member);
        char *java_type = java_type_name(member->type_spec, false);
        sb_appendf(sb, "    public %s %s;\n", java_type, name);
        free(java_type);
        idl_node_t *next_node = (idl_node_t *)member;
        if (!next_node->next) break;
        member = (const idl_member_t *)next_node->next;
    }
    sb_append(sb, "\n");
    return 0;
}

static int generate_describe_type(string_builder_t *sb, const idl_struct_t *struct_def, const char *class_name) {
    sb_appendf(sb, "    public static DynamicType describeType() {\n");
    sb_appendf(sb, "        DynamicType dt = new DynamicType(\"%s\");\n", class_name);
    
    for (const idl_member_t *member = struct_def->members; member; ) {
        const char *name = get_member_name(member);
        idl_type_t type = resolve_typedef_type(member->type_spec);
        const char *kind = dynamic_type_kind(type);
        sb_appendf(sb, "        dt.addMember(\"%s\", DynamicType.%s);\n", name, kind);
        idl_node_t *next_node = (idl_node_t *)member;
        if (!next_node->next) break;
        member = (const idl_member_t *)next_node->next;
    }
    
    sb_append(sb, "        return dt;\n");
    sb_append(sb, "    }\n\n");
    return 0;
}

static int generate_serialize_method(string_builder_t *sb, const idl_struct_t *struct_def, const char *class_name) {
    sb_append(sb, "    public byte[] serialize() {\n");
    sb_append(sb, "        ByteBuffer buffer = ByteBuffer.allocate(256);\n");
    sb_append(sb, "        buffer.order(ByteOrder.LITTLE_ENDIAN);\n\n");
    
    for (const idl_member_t *member = struct_def->members; member; ) {
        const char *name = get_member_name(member);
        // Resolve typedef to get the underlying type for serialization
        idl_type_t type = resolve_typedef_type(member->type_spec);
        
        switch (type) {
            case IDL_BOOL:
            case IDL_OCTET:
            case IDL_CHAR:
                sb_appendf(sb, "        buffer.put(%s);\n", name);
                break;
            case IDL_SHORT:
            case IDL_USHORT:
                sb_appendf(sb, "        buffer.putShort(%s);\n", name);
                break;
            case IDL_LONG:
            case IDL_ULONG:
                sb_appendf(sb, "        buffer.putInt(%s);\n", name);
                break;
            case IDL_LLONG:
            case IDL_ULLONG:
                sb_appendf(sb, "        buffer.putLong(%s);\n", name);
                break;
            case IDL_FLOAT:
                sb_appendf(sb, "        buffer.putFloat(%s);\n", name);
                break;
            case IDL_DOUBLE:
                sb_appendf(sb, "        buffer.putDouble(%s);\n", name);
                break;
            case IDL_STRING:
                sb_appendf(sb, "        if (%s != null) {\n", name);
                sb_appendf(sb, "            byte[] bytes = %s.getBytes(StandardCharsets.UTF_8);\n", name);
                sb_append(sb, "            buffer.putInt(bytes.length);\n");
                sb_append(sb, "            buffer.put(bytes);\n");
                sb_append(sb, "        } else {\n");
                sb_append(sb, "            buffer.putInt(-1);\n");
                sb_append(sb, "        }\n");
                break;
            case IDL_SEQUENCE: {
                const idl_sequence_t *seq = (const idl_sequence_t *)member->type_spec;
                char *elem_type = java_type_name(seq->type_spec, true);
                idl_type_t elem_idl_type = resolve_typedef_type(seq->type_spec);
                sb_appendf(sb, "        if (%s != null) {\n", name);
                sb_appendf(sb, "            buffer.putInt(%s.size());\n", name);
                sb_appendf(sb, "            for (%s elem : %s) {\n", elem_type, name);
                // Serialize based on element type
                switch (elem_idl_type) {
                    case IDL_BOOL:
                    case IDL_OCTET:
                    case IDL_CHAR:
                        sb_append(sb, "                buffer.put(elem);\n");
                        break;
                    case IDL_SHORT:
                    case IDL_USHORT:
                        sb_append(sb, "                buffer.putShort(elem);\n");
                        break;
                    case IDL_LONG:
                    case IDL_ULONG:
                        sb_append(sb, "                buffer.putInt(elem);\n");
                        break;
                    case IDL_LLONG:
                    case IDL_ULLONG:
                        sb_append(sb, "                buffer.putLong(elem);\n");
                        break;
                    case IDL_FLOAT:
                        sb_append(sb, "                buffer.putFloat(elem);\n");
                        break;
                    case IDL_DOUBLE:
                        sb_append(sb, "                buffer.putDouble(elem);\n");
                        break;
                    case IDL_STRING:
                        sb_append(sb, "                if (elem != null) {\n");
                        sb_append(sb, "                    byte[] elemBytes = elem.getBytes(StandardCharsets.UTF_8);\n");
                        sb_append(sb, "                    buffer.putInt(elemBytes.length);\n");
                        sb_append(sb, "                    buffer.put(elemBytes);\n");
                        sb_append(sb, "                } else {\n");
                        sb_append(sb, "                    buffer.putInt(-1);\n");
                        sb_append(sb, "                }\n");
                        break;
                    case IDL_STRUCT:
                        sb_append(sb, "                if (elem != null) {\n");
                        sb_append(sb, "                    buffer.put(elem.serialize());\n");
                        sb_append(sb, "                }\n");
                        break;
                    case IDL_ENUM:
                        sb_append(sb, "                buffer.putInt(elem.getValue());\n");
                        break;
                    case IDL_UNION:
                        sb_append(sb, "                if (elem != null) {\n");
                        sb_append(sb, "                    buffer.put(elem.serialize());\n");
                        sb_append(sb, "                }\n");
                        break;
                    case IDL_BITMASK:
                        sb_append(sb, "                buffer.putLong(elem.getValue());\n");
                        break;
                    case IDL_WSTRING:
                        sb_append(sb, "                if (elem != null) {\n");
                        sb_append(sb, "                    byte[] elemBytes = elem.getBytes(java.nio.charset.StandardCharsets.UTF_8);\n");
                        sb_append(sb, "                    buffer.putInt(elemBytes.length);\n");
                        sb_append(sb, "                    buffer.put(elemBytes);\n");
                        sb_append(sb, "                } else {\n");
                        sb_append(sb, "                    buffer.putInt(-1);\n");
                        sb_append(sb, "                }\n");
                        break;
                    default:
                        sb_appendf(sb, "                // TODO: serialize %s element\n", elem_type);
                        break;
                }
                sb_append(sb, "            }\n");
                sb_append(sb, "        } else {\n");
                sb_append(sb, "            buffer.putInt(-1);\n");
                sb_append(sb, "        }\n");
                free(elem_type);
                break;
            }
            case IDL_STRUCT:
                sb_appendf(sb, "        if (%s != null) {\n", name);
                sb_appendf(sb, "            buffer.put(%s.serialize());\n", name);
                sb_append(sb, "        }\n");
                break;
            case IDL_UNION:
                sb_appendf(sb, "        if (%s != null) {\n", name);
                sb_appendf(sb, "            buffer.put(%s.serialize());\n", name);
                sb_append(sb, "        }\n");
                break;
            case IDL_ENUM:
                sb_appendf(sb, "        buffer.putInt(%s.getValue());\n", name);
                break;
            default:
                // Unknown type - skip for now
                break;
        }
        
        idl_node_t *next_node = (idl_node_t *)member;
        if (!next_node->next) break;
        member = (const idl_member_t *)next_node->next;
    }
    
    sb_append(sb, "\n        byte[] result = new byte[buffer.position()];\n");
    sb_append(sb, "        buffer.flip();\n");
    sb_append(sb, "        buffer.get(result);\n");
    sb_append(sb, "        return result;\n");
    sb_append(sb, "    }\n\n");
    return 0;
}

static int generate_deserialize_method(string_builder_t *sb, const idl_struct_t *struct_def) {
    sb_append(sb, "    public void deserialize(byte[] data) {\n");
    sb_append(sb, "        ByteBuffer buffer = ByteBuffer.wrap(data);\n");
    sb_append(sb, "        buffer.order(ByteOrder.LITTLE_ENDIAN);\n\n");

    for (const idl_member_t *member = struct_def->members; member; ) {
        const char *name = get_member_name(member);
        // Resolve typedef to get the underlying type for deserialization
        idl_type_t type = resolve_typedef_type(member->type_spec);

        switch (type) {
            case IDL_BOOL:
            case IDL_OCTET:
            case IDL_CHAR:
                sb_appendf(sb, "        %s = buffer.get();\n", name);
                break;
            case IDL_SHORT:
            case IDL_USHORT:
                sb_appendf(sb, "        %s = buffer.getShort();\n", name);
                break;
            case IDL_LONG:
            case IDL_ULONG:
                sb_appendf(sb, "        %s = buffer.getInt();\n", name);
                break;
            case IDL_LLONG:
            case IDL_ULLONG:
                sb_appendf(sb, "        %s = buffer.getLong();\n", name);
                break;
            case IDL_FLOAT:
                sb_appendf(sb, "        %s = buffer.getFloat();\n", name);
                break;
            case IDL_DOUBLE:
                sb_appendf(sb, "        %s = buffer.getDouble();\n", name);
                break;
            case IDL_STRING:
                sb_append(sb, "        {\n");
                sb_append(sb, "            int len = buffer.getInt();\n");
                sb_append(sb, "            if (len > 0) {\n");
                sb_appendf(sb, "                %s = new String(buffer.array(), buffer.position(), len, StandardCharsets.UTF_8);\n", name);
                sb_append(sb, "                buffer.position(buffer.position() + len);\n");
                sb_append(sb, "            } else {\n");
                sb_appendf(sb, "                %s = null;\n", name);
                sb_append(sb, "            }\n");
                sb_append(sb, "        }\n");
                break;
            case IDL_SEQUENCE: {
                const idl_sequence_t *seq = (const idl_sequence_t *)member->type_spec;
                char *elem_type = java_type_name(seq->type_spec, true);
                idl_type_t elem_idl_type = resolve_typedef_type(seq->type_spec);
                sb_append(sb, "        {\n");
                sb_append(sb, "            int len = buffer.getInt();\n");
                sb_appendf(sb, "            %s = new java.util.ArrayList<>();\n", name);
                sb_append(sb, "            for (int i = 0; i < len; i++) {\n");
                // Deserialize based on element type
                switch (elem_idl_type) {
                    case IDL_BOOL:
                    case IDL_OCTET:
                    case IDL_CHAR:
                        sb_appendf(sb, "                %s.add(buffer.get());\n", name);
                        break;
                    case IDL_SHORT:
                    case IDL_USHORT:
                        sb_appendf(sb, "                %s.add(buffer.getShort());\n", name);
                        break;
                    case IDL_LONG:
                    case IDL_ULONG:
                        sb_appendf(sb, "                %s.add(buffer.getInt());\n", name);
                        break;
                    case IDL_LLONG:
                    case IDL_ULLONG:
                        sb_appendf(sb, "                %s.add(buffer.getLong());\n", name);
                        break;
                    case IDL_FLOAT:
                        sb_appendf(sb, "                %s.add(buffer.getFloat());\n", name);
                        break;
                    case IDL_DOUBLE:
                        sb_appendf(sb, "                %s.add(buffer.getDouble());\n", name);
                        break;
                    case IDL_STRING:
                        sb_appendf(sb, "                int elemLen = buffer.getInt();\n");
                        sb_appendf(sb, "                if (elemLen > 0) {\n");
                        sb_appendf(sb, "                    String elemStr = new String(buffer.array(), buffer.position(), elemLen, StandardCharsets.UTF_8);\n");
                        sb_appendf(sb, "                    buffer.position(buffer.position() + elemLen);\n");
                        sb_appendf(sb, "                    %s.add(elemStr);\n");
                        sb_appendf(sb, "                } else {\n");
                        sb_appendf(sb, "                    %s.add(null);\n");
                        sb_appendf(sb, "                }\n");
                        break;
                    case IDL_STRUCT: {
                        char *boxed_elem = java_type_name(seq->type_spec, true);
                        sb_appendf(sb, "                %s elem = new %s();\n", boxed_elem, boxed_elem);
                        sb_appendf(sb, "                byte[] elemData = new byte[buffer.remaining()];\n");
                        sb_appendf(sb, "                buffer.get(elemData);\n");
                        sb_appendf(sb, "                elem.deserialize(elemData);\n");
                        sb_appendf(sb, "                %s.add(elem);\n", name);
                        free(boxed_elem);
                        break;
                    }
                    case IDL_ENUM: {
                        sb_appendf(sb, "                int enumVal = buffer.getInt();\n");
                        sb_appendf(sb, "                %s elemEnum = %s.valueOf(enumVal);\n", elem_type, elem_type);
                        sb_appendf(sb, "                %s.add(elemEnum);\n", name);
                        break;
                    }
                    case IDL_UNION: {
                        char *boxed_elem = java_type_name(seq->type_spec, true);
                        sb_appendf(sb, "                %s elem = new %s();\n", boxed_elem, boxed_elem);
                        sb_appendf(sb, "                byte[] elemData = new byte[buffer.remaining()];\n");
                        sb_appendf(sb, "                buffer.get(elemData);\n");
                        sb_appendf(sb, "                elem.deserialize(elemData);\n");
                        sb_appendf(sb, "                %s.add(elem);\n", name);
                        free(boxed_elem);
                        break;
                    }
                    case IDL_BITMASK: {
                        sb_appendf(sb, "                long bitmaskVal = buffer.getLong();\n");
                        sb_appendf(sb, "                %s elemMask = new %s(bitmaskVal);\n", elem_type, elem_type);
                        sb_appendf(sb, "                %s.add(elemMask);\n", name);
                        break;
                    }
                    case IDL_WSTRING: {
                        sb_appendf(sb, "                int wstrLen = buffer.getInt();\n");
                        sb_appendf(sb, "                if (wstrLen > 0) {\n");
                        sb_appendf(sb, "                    byte[] wstrBytes = new byte[wstrLen];\n");
                        sb_appendf(sb, "                    buffer.get(wstrBytes);\n");
                        sb_appendf(sb, "                    String wstr = new String(wstrBytes, java.nio.charset.StandardCharsets.UTF_8);\n");
                        sb_appendf(sb, "                    %s.add(wstr);\n", name);
                        sb_appendf(sb, "                } else {\n");
                        sb_appendf(sb, "                    %s.add(null);\n", name);
                        sb_appendf(sb, "                }\n");
                        break;
                    }
                    default:
                        sb_appendf(sb, "                // TODO: deserialize %s element\n", elem_type);
                        break;
                }
                sb_append(sb, "            }\n");
                sb_append(sb, "        }\n");
                free(elem_type);
                break;
            }
            case IDL_STRUCT: {
                char *struct_type = java_type_name(member->type_spec, false);
                sb_appendf(sb, "        if (%s == null) {\n", name);
                sb_appendf(sb, "            %s = new %s();\n", name, struct_type);
                sb_append(sb, "        }\n");
                sb_appendf(sb, "        byte[] %s_data = new byte[buffer.remaining()];\n", name);
                sb_appendf(sb, "        buffer.get(%s_data);\n", name);
                sb_appendf(sb, "        %s.deserialize(%s_data);\n", name, name);
                free(struct_type);
                break;
            }
            case IDL_UNION: {
                char *union_type = java_type_name(member->type_spec, false);
                sb_appendf(sb, "        if (%s == null) {\n", name);
                sb_appendf(sb, "            %s = new %s();\n", name, union_type);
                sb_append(sb, "        }\n");
                sb_appendf(sb, "        byte[] %s_data = new byte[buffer.remaining()];\n", name);
                sb_appendf(sb, "        buffer.get(%s_data);\n", name);
                sb_appendf(sb, "        %s.deserialize(%s_data);\n", name, name);
                free(union_type);
                break;
            }
            case IDL_ENUM: {
                char *enum_type = java_type_name(member->type_spec, false);
                sb_appendf(sb, "        int %s_val = buffer.getInt();\n", name);
                sb_appendf(sb, "        %s = %s.valueOf(%s_val);\n", name, enum_type, name);
                free(enum_type);
                break;
            }
            default:
                // Unknown type - skip for now
                break;
        }
        
        idl_node_t *next_node = (idl_node_t *)member;
        if (!next_node->next) break;
        member = (const idl_member_t *)next_node->next;
    }
    
    sb_append(sb, "    }\n\n");
    return 0;
}

static int generate_to_string(string_builder_t *sb, const idl_struct_t *struct_def, const char *class_name) {
    sb_append(sb, "    @Override\n");
    sb_append(sb, "    public String toString() {\n");
    sb_appendf(sb, "        return \"%s[\" +\n", class_name);
    
    int field_count = 0;
    for (const idl_member_t *member = struct_def->members; member; ) {
        const char *name = get_member_name(member);
        if (field_count > 0) {
            sb_append(sb, " +\n");
            sb_appendf(sb, "            \", %s=\" + %s", name, name);
        } else {
            sb_appendf(sb, "            \"%s=\" + %s", name, name);
        }
        field_count++;
        idl_node_t *next_node = (idl_node_t *)member;
        if (!next_node->next) break;
        member = (const idl_member_t *)next_node->next;
    }
    
    sb_append(sb, " +\n            \"]\";\n");
    sb_append(sb, "    }\n");
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
    
    generate_structure_header(sb, package, actual_class_name);
    generate_field_order(sb, struct_def);
    generate_structure_fields(sb, struct_def);
    generate_describe_type(sb, struct_def, actual_class_name);
    
    if (!disable_cdr) {
        generate_serialize_method(sb, struct_def, actual_class_name);
        generate_deserialize_method(sb, struct_def);
    }
    
    generate_to_string(sb, struct_def, actual_class_name);
    sb_append(sb, "}\n");
    
    // Create directory structure
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
    
    // Write file
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
    sb_append(sb, "import com.sun.jna.Structure;\n\n");
    sb_appendf(sb, "public enum %s {\n", actual_enum_name);
    
    int enum_count = 0;
    for (const idl_enumerator_t *enumerator = enum_def->enumerators; enumerator; 
         enumerator = (const idl_enumerator_t *)((idl_node_t *)enumerator)->next) {
        if (enum_count > 0) sb_append(sb, ",\n");
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
    sb_append(sb, "    }\n\n");
    
    sb_append(sb, "    public static DynamicType describeType() {\n");
    sb_appendf(sb, "        DynamicType dt = new DynamicType(\"%s\");\n", actual_enum_name);
    sb_append(sb, "        dt.setKind(DynamicType.ENUM);\n");
    
    for (const idl_enumerator_t *enumerator = enum_def->enumerators; enumerator; 
         enumerator = (const idl_enumerator_t *)((idl_node_t *)enumerator)->next) {
        const char *enum_value = "VALUE";
        if (enumerator->name && enumerator->name->identifier) {
            enum_value = enumerator->name->identifier;
        }
        sb_appendf(sb, "        dt.addEnumerator(\"%s\");\n", enum_value);
    }
    
    sb_append(sb, "        return dt;\n");
    sb_append(sb, "    }\n");
    sb_append(sb, "}\n");
    
    // Create directory and write file
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
    sb_append(sb, "import com.sun.jna.Structure;\n\n");
    sb_appendf(sb, "public class %s extends Structure {\n\n", typedef_name);
    sb_appendf(sb, "    public %s value;\n\n", java_type);
    sb_appendf(sb, "    public %s() { }\n\n", typedef_name);
    sb_appendf(sb, "    public %s(%s value) {\n", typedef_name, java_type);
    sb_append(sb, "        this.value = value;\n");
    sb_append(sb, "    }\n\n");
    sb_appendf(sb, "    public %s getValue() { return value; }\n", java_type);
    sb_appendf(sb, "    public void setValue(%s value) { this.value = value; }\n", java_type);
    sb_append(sb, "}\n");
    
    // Create directory and write file
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

int generate_java_union(const idl_union_t *union_def, const char *output_dir, const char *prefix, const char *union_name) {
    if (!union_def || !output_dir) return -1;
    
    char *package = resolve_package((const idl_node_t *)union_def, prefix);
    const char *actual_union_name = union_name ? union_name : "GeneratedUnion";
    
    string_builder_t *sb = sb_create();
    if (!sb) {
        free(package);
        return -1;
    }
    
    // Get discriminator type name
    const char *discrim_type = "int";
    if (union_def->switch_type_spec && union_def->switch_type_spec->type_spec) {
        idl_type_t dt = idl_type(union_def->switch_type_spec->type_spec);
        switch (dt) {
            case IDL_CHAR: case IDL_OCTET: case IDL_BOOL: discrim_type = "byte"; break;
            case IDL_SHORT: case IDL_USHORT: discrim_type = "short"; break;
            case IDL_LONG: case IDL_ULONG: discrim_type = "int"; break;
            case IDL_LLONG: case IDL_ULLONG: discrim_type = "long"; break;
            default: discrim_type = "int"; break;
        }
    }
    
    sb_appendf(sb, "package %s;\n\n", package);
    sb_append(sb, "import com.sun.jna.Structure;\n");
    sb_append(sb, "import java.nio.ByteBuffer;\n");
    sb_append(sb, "import java.nio.ByteOrder;\n\n");
    sb_appendf(sb, "public class %s extends Structure {\n\n", actual_union_name);
    
    // Add discriminator field
    sb_appendf(sb, "    public %s _d;  // union discriminator\n\n", discrim_type);
    
    // Add union case fields
    sb_append(sb, "    // Union case fields\n");
    for (const idl_case_t *case_def = union_def->cases; case_def; case_def = (const idl_case_t *)((const idl_node_t *)case_def)->next) {
        if (case_def->declarator && case_def->declarator->name && case_def->declarator->name->identifier) {
            const char *field_name = case_def->declarator->name->identifier;
            char *field_type = java_type_name(case_def->type_spec, false);
            sb_appendf(sb, "    public %s %s;\n", field_type, field_name);
            free(field_type);
        }
    }
    sb_append(sb, "\n");
    
    // Add helper methods for setting cases
    sb_append(sb, "    // Set union value based on discriminator\n");
    for (const idl_case_t *case_def = union_def->cases; case_def; case_def = (const idl_case_t *)((const idl_node_t *)case_def)->next) {
        if (case_def->declarator && case_def->declarator->name && case_def->declarator->name->identifier) {
            const char *field_name = case_def->declarator->name->identifier;
            // Capitalize first letter
            char *setter_name = malloc(strlen(field_name) + 4);
            sprintf(setter_name, "set%c%s", toupper(field_name[0]), field_name + 1);
            char *field_type = java_type_name(case_def->type_spec, false);
            sb_appendf(sb, "    public void %s(%s value) {\n", setter_name, field_type);
            sb_appendf(sb, "        this.%s = value;\n", field_name);
            sb_append(sb, "    }\n");
            free(setter_name);
            free(field_type);
        }
    }
    sb_append(sb, "\n");
    
    sb_append(sb, "    public static DynamicType describeType() {\n");
    sb_appendf(sb, "        DynamicType dt = new DynamicType(\"%s\");\n", actual_union_name);
    sb_append(sb, "        dt.setKind(DynamicType.UNION);\n");
    sb_appendf(sb, "        dt.addMember(\"_d\", DynamicType.INT%d);\n", (strcmp(discrim_type, "long") == 0) ? 64 : 32);
    sb_append(sb, "        return dt;\n");
    sb_append(sb, "    }\n\n");
    
    sb_append(sb, "    public byte[] serialize() {\n");
    sb_append(sb, "        ByteBuffer buffer = ByteBuffer.allocate(256);\n");
    sb_append(sb, "        buffer.order(ByteOrder.LITTLE_ENDIAN);\n");
    sb_appendf(sb, "        buffer.put%s(_d);\n", (strcmp(discrim_type, "long") == 0) ? "Long" : "Int");
    // Serialize all non-null case fields
    for (const idl_case_t *case_def = union_def->cases; case_def; case_def = (const idl_case_t *)((const idl_node_t *)case_def)->next) {
        if (case_def->declarator && case_def->declarator->name && case_def->declarator->name->identifier) {
            const char *field_name = case_def->declarator->name->identifier;
            sb_appendf(sb, "        if (%s != null) {\n", field_name);
            sb_appendf(sb, "            buffer.put(%s.serialize());\n", field_name);
            sb_append(sb, "        }\n");
        }
    }
    sb_append(sb, "        byte[] result = new byte[buffer.position()];\n");
    sb_append(sb, "        buffer.flip();\n");
    sb_append(sb, "        buffer.get(result);\n");
    sb_append(sb, "        return result;\n");
    sb_append(sb, "    }\n\n");
    
    sb_append(sb, "    public void deserialize(byte[] data) {\n");
    sb_append(sb, "        ByteBuffer buffer = ByteBuffer.wrap(data);\n");
    sb_append(sb, "        buffer.order(ByteOrder.LITTLE_ENDIAN);\n");
    sb_appendf(sb, "        _d = buffer.get%s();\n", (strcmp(discrim_type, "long") == 0) ? "Long" : "Int");
    // For now, just read remaining data into first non-null case field
    sb_append(sb, "        byte[] remaining = new byte[buffer.remaining()];\n");
    sb_append(sb, "        buffer.get(remaining);\n");
    // Try to deserialize into each case field
    for (const idl_case_t *case_def = union_def->cases; case_def; case_def = (const idl_case_t *)((const idl_node_t *)case_def)->next) {
        if (case_def->declarator && case_def->declarator->name && case_def->declarator->name->identifier) {
            const char *field_name = case_def->declarator->name->identifier;
            char *field_type = java_type_name(case_def->type_spec, false);
            sb_appendf(sb, "        // Try to deserialize into %s\n", field_name);
            sb_appendf(sb, "        %s = new %s();\n", field_name, field_type);
            sb_appendf(sb, "        %s.deserialize(remaining);\n", field_name);
            free(field_type);
            break;
        }
    }
    sb_append(sb, "    }\n\n");
    
    sb_append(sb, "    @Override\n");
    sb_append(sb, "    public String toString() {\n");
    sb_appendf(sb, "        return \"%s[\" + _d + \"]\";\n", actual_union_name);
    sb_append(sb, "    }\n");
    sb_append(sb, "}\n");
    
    // Create directory and write file
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
    snprintf(file_path, sizeof(file_path), "%s/%s.java", package_path, actual_union_name);
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

int generate_java_bitmask(const idl_bitmask_t *bitmask_def, const char *output_dir, const char *prefix, const char *bitmask_name) {
    if (!bitmask_def || !output_dir) return -1;
    
    char *package = resolve_package((const idl_node_t *)bitmask_def, prefix);
    const char *actual_bitmask_name = bitmask_name ? bitmask_name : "GeneratedBitmask";
    
    string_builder_t *sb = sb_create();
    if (!sb) {
        free(package);
        return -1;
    }
    
    sb_appendf(sb, "package %s;\n\n", package);
    sb_append(sb, "import com.sun.jna.Structure;\n\n");
    sb_appendf(sb, "public class %s extends Structure {\n\n", actual_bitmask_name);
    
    // Add bitmask value field
    sb_append(sb, "    public long value;  // bitmask value\n\n");
    
    // Add bit values as constants
    sb_append(sb, "    // Bit values\n");
    int bit_pos = 0;
    for (const idl_bit_value_t *bit = bitmask_def->bit_values; bit; bit = (const idl_bit_value_t *)((idl_node_t *)bit)->next) {
        const char *bit_name = "BIT";
        if (bit->name && bit->name->identifier) {
            bit_name = bit->name->identifier;
        }
        sb_appendf(sb, "    public static final long %s = 1L << %d;\n", bit_name, bit_pos);
        bit_pos++;
    }
    sb_append(sb, "\n");
    
    sb_appendf(sb, "    public %s() { }\n\n", actual_bitmask_name);
    sb_appendf(sb, "    public %s(long value) {\n", actual_bitmask_name);
    sb_append(sb, "        this.value = value;\n");
    sb_append(sb, "    }\n\n");
    
    sb_append(sb, "    public long getValue() { return value; }\n");
    sb_append(sb, "    public void setValue(long value) { this.value = value; }\n\n");
    
    // Add helper methods
    sb_appendf(sb, "    public boolean isSet(long flag) {\n");
    sb_append(sb, "        return (value & flag) == flag;\n");
    sb_append(sb, "    }\n\n");
    
    sb_appendf(sb, "    public void setFlag(long flag) {\n");
    sb_append(sb, "        value |= flag;\n");
    sb_append(sb, "    }\n\n");
    
    sb_appendf(sb, "    public void clearFlag(long flag) {\n");
    sb_append(sb, "        value &= ~flag;\n");
    sb_append(sb, "    }\n\n");
    
    sb_append(sb, "    public static DynamicType describeType() {\n");
    sb_appendf(sb, "        DynamicType dt = new DynamicType(\"%s\");\n", actual_bitmask_name);
    sb_append(sb, "        dt.setKind(DynamicType.BITMASK);\n");
    sb_append(sb, "        dt.addMember(\"value\", DynamicType.INT64);\n");
    sb_append(sb, "        return dt;\n");
    sb_append(sb, "    }\n\n");
    
    sb_append(sb, "    @Override\n");
    sb_append(sb, "    public String toString() {\n");
    sb_appendf(sb, "        return \"%s[value=0x\" + Long.toHexString(value) + \"]\";\n", actual_bitmask_name);
    sb_append(sb, "    }\n");
    sb_append(sb, "}\n");
    
    // Create directory and write file
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
    snprintf(file_path, sizeof(file_path), "%s/%s.java", package_path, actual_bitmask_name);
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
