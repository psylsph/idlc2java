#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "idlc_java.h"

typedef struct string_builder string_builder_t;

extern string_builder_t *sb_create(void);
extern void sb_destroy(string_builder_t *sb);
extern int sb_append(string_builder_t *sb, const char *str);
extern int sb_appendf(string_builder_t *sb, const char *format, ...);
extern const char *sb_string(const string_builder_t *sb);

extern char *java_type_name(const idl_type_spec_t *type_spec, bool boxed);

static int generate_primitive_read(string_builder_t *sb, idl_type_t type, const char *var_name) {
    switch (type) {
        case IDL_BOOL:
            sb_appendf(sb, "input.read() != 0");
            break;
        case IDL_OCTET:
        case IDL_CHAR:
            sb_appendf(sb, "(byte) input.read()");
            break;
        case IDL_SHORT:
            sb_appendf(sb, "input.readShort()");
            break;
        case IDL_USHORT:
            sb_appendf(sb, "input.readUnsignedShort()");
            break;
        case IDL_LONG:
        case IDL_ULONG:
            sb_appendf(sb, "input.readInt()");
            break;
        case IDL_LLONG:
        case IDL_ULLONG:
            sb_appendf(sb, "input.readLong()");
            break;
        case IDL_FLOAT:
            sb_appendf(sb, "input.readFloat()");
            break;
        case IDL_DOUBLE:
            sb_appendf(sb, "input.readDouble()");
            break;
        default:
            return -1;
    }
    return 0;
}

static int generate_primitive_write(string_builder_t *sb, idl_type_t type, const char *var_name) {
    switch (type) {
        case IDL_BOOL:
            sb_appendf(sb, "output.write(%s ? 1 : 0)", var_name);
            break;
        case IDL_OCTET:
        case IDL_CHAR:
            sb_appendf(sb, "output.write(%s)", var_name);
            break;
        case IDL_SHORT:
            sb_appendf(sb, "output.writeShort(%s)", var_name);
            break;
        case IDL_USHORT:
            sb_appendf(sb, "output.writeShort(%s)", var_name);
            break;
        case IDL_LONG:
        case IDL_ULONG:
            sb_appendf(sb, "output.writeInt(%s)", var_name);
            break;
        case IDL_LLONG:
        case IDL_ULLONG:
            sb_appendf(sb, "output.writeLong(%s)", var_name);
            break;
        case IDL_FLOAT:
            sb_appendf(sb, "output.writeFloat(%s)", var_name);
            break;
        case IDL_DOUBLE:
            sb_appendf(sb, "output.writeDouble(%s)", var_name);
            break;
        default:
            return -1;
    }
    return 0;
}

int generate_cdr_reader(const idl_type_spec_t *type_spec, const char *var_name, string_builder_t *sb) {
    if (!type_spec || !sb) return -1;
    
    idl_mask_t mask = idl_mask(type_spec);
    
    if (mask & IDL_BASE_TYPE) {
        return generate_primitive_read(sb, (idl_type_t)mask, var_name);
    } else if (mask & IDL_STRING) {
        sb_append(sb, "readString(input)");
        return 0;
    } else if (mask & IDL_SEQUENCE) {
        sb_append(sb, "readSequence(input)");
        return 0;
    } else if (mask & (IDL_STRUCT | IDL_UNION)) {
        const char *type_name = NULL;
        if (mask & IDL_STRUCT) {
            type_name = idl_identifier(((const idl_struct_t *)type_spec)->name);
        } else {
            type_name = idl_identifier(((const idl_union_t *)type_spec)->name);
        }
        sb_appendf(sb, "%s.readCDR(input)", type_name);
        return 0;
    }
    
    return -1;
}

int generate_cdr_writer(const idl_type_spec_t *type_spec, const char *var_name, string_builder_t *sb) {
    if (!type_spec || !sb) return -1;
    
    idl_mask_t mask = idl_mask(type_spec);
    
    if (mask & IDL_BASE_TYPE) {
        return generate_primitive_write(sb, (idl_type_t)mask, var_name);
    } else if (mask & IDL_STRING) {
        sb_appendf(sb, "writeString(output, %s)", var_name);
        return 0;
    } else if (mask & IDL_SEQUENCE) {
        sb_appendf(sb, "writeSequence(output, %s)", var_name);
        return 0;
    } else if (mask & (IDL_STRUCT | IDL_UNION)) {
        sb_appendf(sb, "%s.writeCDR(output)", var_name);
        return 0;
    }
    
    return -1;
}

int generate_cdr_helpers(string_builder_t *sb, const char *class_name) {
    sb_appendf(sb, "    private static String readString(InputStream input) throws IOException {\n");
    sb_appendf(sb, "        int len = input.readInt();\n");
    sb_appendf(sb, "        byte[] bytes = new byte[len];\n");
    sb_appendf(sb, "        input.readFully(bytes);\n");
    sb_appendf(sb, "        return new String(bytes, \"UTF-8\");\n");
    sb_appendf(sb, "    }\n\n");
    
    sb_appendf(sb, "    private static void writeString(OutputStream output, String str) throws IOException {\n");
    sb_appendf(sb, "        byte[] bytes = str.getBytes(\"UTF-8\");\n");
    sb_appendf(sb, "        output.writeInt(bytes.length);\n");
    sb_appendf(sb, "        output.write(bytes);\n");
    sb_appendf(sb, "    }\n\n");
    
    sb_appendf(sb, "    private static <T> List<T> readSequence(InputStream input) throws IOException {\n");
    sb_appendf(sb, "        int len = input.readInt();\n");
    sb_appendf(sb, "        List<T> list = new ArrayList<>(len);\n");
    sb_appendf(sb, "        for (int i = 0; i < len; i++) {\n");
    sb_appendf(sb, "            // TODO: read element\n");
    sb_appendf(sb, "        }\n");
    sb_appendf(sb, "        return list;\n");
    sb_appendf(sb, "    }\n\n");
    
    sb_appendf(sb, "    private static <T> void writeSequence(OutputStream output, List<T> list) throws IOException {\n");
    sb_appendf(sb, "        output.writeInt(list.size());\n");
    sb_appendf(sb, "        for (T item : list) {\n");
    sb_appendf(sb, "            // TODO: write element\n");
    sb_appendf(sb, "        }\n");
    sb_appendf(sb, "    }\n\n");
    
    return 0;
}
