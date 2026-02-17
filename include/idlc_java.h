#ifndef IDLC_JAVA_H
#define IDLC_JAVA_H

#include "idlc/generator.h"
#include "idl/processor.h"
#include "idl/tree.h"
#include "idl/visit.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct java_generator_config {
    char *output_dir;
    char *package_prefix;
    bool use_arrays_for_sequences;
    bool disable_cdr;
    bool generate_records;
} java_generator_config_t;

typedef struct java_type_map {
    const idl_type_spec_t *type_spec;
    char *java_name;
    char *java_boxed_name;
    bool is_primitive;
    bool is_string;
} java_type_map_t;

#ifdef __cplusplus
}
#endif

int generate_java_record(const idl_struct_t *struct_def, const char *output_dir, const char *prefix, bool disable_cdr, const char *class_name);
int generate_java_enum(const idl_enum_t *enum_def, const char *output_dir, const char *prefix, const char *enum_name);
int generate_java_typedef(const idl_typedef_t *typedef_def, const char *output_dir, const char *prefix);
char *resolve_package(const idl_node_t *node, const char *prefix);
const char *get_struct_name(const idl_struct_t *struct_def);
char *java_type_name(const idl_type_spec_t *type_spec, bool boxed);

#endif /* IDLC_JAVA_H */
