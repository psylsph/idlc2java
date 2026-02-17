#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "idlc_java.h"

/* Get struct name - try different ways to safely extract the name */
const char *get_struct_name(const idl_struct_t *struct_def) {
    if (!struct_def) return "UnknownStruct";
    
    /* Method 1: Try to access the name directly if it's a pointer to a string */
    /* The idl_name structure typically has an 'identifier' field */
    /* We'll try different offsets based on common IDL compiler layouts */
    
    /* For CycloneDDS, the name is typically stored as a pointer after the node */
    /* Let's try dereferencing what looks like a pointer to char* */
    const char **name_ptr_ptr = (const char **)((char*)struct_def + sizeof(void*));
    const char *name_ptr = *name_ptr_ptr;
    
    /* If it looks like a valid string (printable ASCII), use it */
    if (name_ptr && strlen(name_ptr) > 0 && strlen(name_ptr) < 100) {
        /* Check if it looks like an identifier */
        bool looks_valid = true;
        for (const char *p = name_ptr; *p; p++) {
            if (!isalnum(*p) && *p != '_') {
                looks_valid = false;
                break;
            }
        }
        if (looks_valid) {
            return name_ptr;
        }
    }
    
    /* Method 2: Check if name is at a different offset */
    /* Try offset after inherit_spec pointer */
    name_ptr_ptr = (const char **)((char*)struct_def + sizeof(void*) * 2);
    name_ptr = *name_ptr_ptr;
    
    if (name_ptr && strlen(name_ptr) > 0 && strlen(name_ptr) < 100) {
        bool looks_valid = true;
        for (const char *p = name_ptr; *p; p++) {
            if (!isalnum(*p) && *p != '_') {
                looks_valid = false;
                break;
            }
        }
        if (looks_valid) {
            return name_ptr;
        }
    }
    
    return "Struct";
}

/* Get the module name from a module node */
const char *get_module_name(const idl_module_t *module) {
    if (!module) return "unknown";
    
    if (module->name && module->name->identifier) {
        return module->name->identifier;
    }
    
    return "module";
}

char *resolve_package(const idl_node_t *node, const char *prefix) {
    /* Build full module path by walking up the parent chain */
    const char *module_names[20];
    int module_count = 0;
    
    const idl_node_t *n = node;
    while (n && module_count < 20) {
        if (idl_mask(n) & IDL_MODULE) {
            const idl_module_t *module = (const idl_module_t *)n;
            if (module->name && module->name->identifier) {
                module_names[module_count++] = module->name->identifier;
            }
        }
        n = n->parent;
    }
    
    if (module_count == 0) {
        char *package_name = strdup("generated");
        if (prefix && strlen(prefix) > 0) {
            char *full = malloc(strlen(prefix) + strlen(package_name) + 2);
            sprintf(full, "%s.%s", prefix, package_name);
            free(package_name);
            return full;
        }
        return package_name;
    }
    
    /* Build package name from module path (reversed since we walked up) */
    int total_len = 0;
    for (int i = module_count - 1; i >= 0; i--) {
        total_len += strlen(module_names[i]) + 1;
    }
    
    char *package_name = malloc(total_len);
    package_name[0] = '\0';
    for (int i = module_count - 1; i >= 0; i--) {
        if (i < module_count - 1) strcat(package_name, ".");
        strcat(package_name, module_names[i]);
    }
    
    if (prefix && strlen(prefix) > 0) {
        char *full = malloc(strlen(prefix) + strlen(package_name) + 2);
        sprintf(full, "%s.%s", prefix, package_name);
        free(package_name);
        return full;
    }
    
    return package_name;
}

char *resolve_simple_name(const idl_node_t *node) {
    if (!node) return strdup("Unnamed");
    
    if (idl_mask(node) & IDL_STRUCT) {
        return strdup(get_struct_name((const idl_struct_t *)node));
    }
    if (idl_mask(node) & IDL_ENUM) {
        const idl_enum_t *e = (const idl_enum_t *)node;
        return e->name ? strdup((const char*)e->name) : strdup("UnnamedEnum");
    }
    
    return strdup("Unnamed");
}

char *resolve_qualified_name(const idl_node_t *node, const char *prefix) {
    char *package = resolve_package(node, prefix);
    char *simple = resolve_simple_name(node);
    
    char *qualified = malloc(strlen(package) + strlen(simple) + 2);
    sprintf(qualified, "%s.%s", package, simple);
    
    free(package);
    free(simple);
    
    return qualified;
}
