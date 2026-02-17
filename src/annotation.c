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
#include "idlc_java.h"

typedef struct string_builder string_builder_t;

extern int sb_append(string_builder_t *sb, const char *str);

static bool has_annotation(const idl_node_t *node, const char *name) {
    if (!node || !node->annotations) return false;
    
    for (const idl_annotation_appl_t *ann = node->annotations; ann; ann = (idl_annotation_appl_t *)ann->node.next) {
        if (ann->annotation && ann->annotation->name) {
            const char *ann_name = idl_identifier(ann->annotation->name);
            if (strcmp(ann_name, name) == 0) {
                return true;
            }
        }
    }
    return false;
}

int process_annotations(const idl_node_t *node, string_builder_t *sb) {
    if (!node || !sb) return 0;
    
    if (!node->annotations) return 0;
    
    for (const idl_annotation_appl_t *ann = node->annotations; ann; ann = (idl_annotation_appl_t *)ann->node.next) {
        if (!ann->annotation || !ann->annotation->name) continue;
        
        const char *ann_name = idl_identifier(ann->annotation->name);
        
        if (strcmp(ann_name, "key") == 0) {
            sb_append(sb, "@Key ");
        } else if (strcmp(ann_name, "optional") == 0) {
            sb_append(sb, "@Optional ");
        } else if (strcmp(ann_name, "id") == 0) {
            sb_append(sb, "@IDLEntity ");
        } else if (strcmp(ann_name, "topic") == 0) {
            sb_append(sb, "@Topic ");
        } else if (strcmp(ann_name, "nested") == 0) {
            sb_append(sb, "@Nested ");
        }
    }
    
    return 0;
}

bool is_nested_type(const idl_node_t *node) {
    return has_annotation(node, "nested");
}

bool is_topic_type(const idl_node_t *node) {
    return has_annotation(node, "topic") || !has_annotation(node, "nested");
}
