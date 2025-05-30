/* vi:set ts=4 sw=4 expandtab:
 *
 * Copyright 2016, Chris Leishman (http://github.com/cleishm)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef NEO4J_PRINT_H
#define NEO4J_PRINT_H

#include "neo4j-client.h"

ssize_t neo4j_null_str(const neo4j_value_t *value, char *buf, size_t n);
ssize_t neo4j_null_fprint(const neo4j_value_t *value, FILE *stream);

ssize_t neo4j_bool_str(const neo4j_value_t *value, char *buf, size_t n);
ssize_t neo4j_bool_fprint(const neo4j_value_t *value, FILE *stream);

ssize_t neo4j_int_str(const neo4j_value_t *value, char *buf, size_t n);
ssize_t neo4j_int_fprint(const neo4j_value_t *value, FILE *stream);

ssize_t neo4j_float_str(const neo4j_value_t *value, char *buf, size_t n);
ssize_t neo4j_float_fprint(const neo4j_value_t *value, FILE *stream);

ssize_t neo4j_string_str(const neo4j_value_t *value, char *buf, size_t n);
ssize_t neo4j_string_fprint(const neo4j_value_t *value, FILE *stream);

ssize_t neo4j_bytes_str(const neo4j_value_t *value, char *buf, size_t n);
ssize_t neo4j_bytes_fprint(const neo4j_value_t *value, FILE *stream);

ssize_t neo4j_list_str(const neo4j_value_t *value, char *buf, size_t n);
ssize_t neo4j_list_fprint(const neo4j_value_t *value, FILE *stream);

ssize_t neo4j_map_str(const neo4j_value_t *value, char *buf, size_t n);
ssize_t neo4j_map_fprint(const neo4j_value_t *value, FILE *stream);

ssize_t neo4j_node_str(const neo4j_value_t *value, char *buf, size_t n);
ssize_t neo4j_node_fprint(const neo4j_value_t *value, FILE *stream);
ssize_t neo4j_rel_str(const neo4j_value_t *value, char *buf, size_t n);
ssize_t neo4j_rel_fprint(const neo4j_value_t *value, FILE *stream);
ssize_t neo4j_path_str(const neo4j_value_t *value, char *buf, size_t n);
ssize_t neo4j_path_fprint(const neo4j_value_t *value, FILE *stream);

ssize_t neo4j_date_str(const neo4j_value_t *value, char *buf, size_t n);
ssize_t neo4j_date_fprint(const neo4j_value_t *value, FILE *stream);

ssize_t neo4j_time_str(const neo4j_value_t *value, char *buf, size_t n);
ssize_t neo4j_time_fprint(const neo4j_value_t *value, FILE *stream);
ssize_t neo4j_localtime_str(const neo4j_value_t *value, char *buf, size_t n);
ssize_t neo4j_localtime_fprint(const neo4j_value_t *value, FILE *stream);
ssize_t neo4j_datetime_str(const neo4j_value_t *value, char *buf, size_t n);
ssize_t neo4j_datetime_fprint(const neo4j_value_t *value, FILE *stream);
ssize_t neo4j_localdatetime_str(const neo4j_value_t *value, char *buf, size_t n);
ssize_t neo4j_localdatetime_fprint(const neo4j_value_t *value, FILE *stream);
ssize_t neo4j_duration_str(const neo4j_value_t *value, char *buf, size_t n);
ssize_t neo4j_duration_fprint(const neo4j_value_t *value, FILE *stream);
ssize_t neo4j_point2d_str(const neo4j_value_t *value, char *buf, size_t n);
ssize_t neo4j_point2d_fprint(const neo4j_value_t *value, FILE *stream);
ssize_t neo4j_point3d_str(const neo4j_value_t *value, char *buf, size_t n);
ssize_t neo4j_point3d_fprint(const neo4j_value_t *value, FILE *stream);



ssize_t neo4j_struct_str(const neo4j_value_t *value, char *buf, size_t n);
ssize_t neo4j_struct_fprint(const neo4j_value_t *value, FILE *stream);

#endif/*NEO4J_PRINT_H*/
