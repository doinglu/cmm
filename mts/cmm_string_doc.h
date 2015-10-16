// cmm_string_doc.h

// ATTENTION: WHY String?
// Since StringImpl is not valid for containers such as hash_map, vector
// because we can't define the class StringImpl but use StringImpl * instead of.
// So we define the type String for using in container.

// How to use the 6 types of string?
// 1. const char *
// 2. const char_t *
// 3. simple::string
// 4. StringImpl *
// 5. Value (when type is ValueType::STRING)
// 6. String
// See following rules:
// 1 - NEVER USE IT UNLESS as function argument (will be constructed to other
//     types)
// 2 - Internal storage pointer type only.
// 3 - DO NOT USE IN THIS PROJECT
// 4 - Implementation of StringImpl in CMM, CAN NOT BE NEW/CONSTRUCT, use
//     StringImpl::new_string and StringImpl:delete_string instead of. 
// 5 - Value can carry a string.
//     We may use this when communicate with upper scripts.
//     Program::invoke()
// 6 - This is derived from Value. It contrains the type as STRING and will
//     be easier to use than Value. It's highly recommended to use it.

#pragma once
