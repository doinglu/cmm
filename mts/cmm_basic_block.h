// cmm_basic_block.h
// Initial version by doing Mar/4/2016
// Define basic block

#pragma once

#include "std_template/simple_vector.h"
#include "cmm.h"

namespace cmm
{

struct AstNode;

// Node no (for build basic blocks)
typedef Uint16 AstNodeNo;

// Variable storage type
enum VarStorage : Uint8
{
    VAR_NONE = 0,
    VAR_ARGUMENT    = 8,     // To argument
    VAR_OBJECT_VAR  = 19,    // To object var
    VAR_LOCAL_VAR   = 47,    // To local var
    VAR_VIRTUAL_REG = 62,    // To a virtual register
};
const char* var_storage_to_c_str(VarStorage storage);

// Block attrib
typedef enum : Uint8
{
    BASIC_BLOCK_NOT_REACHABLE = 0x01, // This block can't reach block#0
} BasicBlockAttrib;
DECLARE_BITS_ENUM(BasicBlockAttrib, Uint8);

// Basic block
typedef Uint16 BasicBlockId;
typedef simple::vector<BasicBlockId> BasicBlockIds;
typedef simple::vector<AstNode*> AstNodes;
struct BasicBlock
{
public:
    // Variable information used in block (input/output)
    typedef Uint16 VersionNo;
    enum
    {
        VERSION_UNINITIALIZED = (VersionNo)-1,
        VERSION_PARTIAL_INITIALIZED = (VersionNo)-2,
    };
    // Is this var possible not initialized?
    // For LOCAL_VAR only
    static bool maybe_uninitialized(VersionNo version_no)
    {
        return version_no == VERSION_UNINITIALIZED ||
               version_no == VERSION_PARTIAL_INITIALIZED;
    }
    static bool is_uninitialized(VersionNo version_no)
    {
        return version_no == VERSION_UNINITIALIZED;
    }
    static bool is_partial_initialized(VersionNo version_no)
    {
        return version_no == VERSION_PARTIAL_INITIALIZED;
    }

    struct VarInfo
    {
        VersionNo  version_no; // Version of variable
        union
        {
            // ATTENTION: sizeof(struct) == sizeof(cmp_val)
            struct
            {
                VariableNo var_no;  // Index of variable
                VarStorage storage; // Variable storage type
                Uint8      unused;  // Unused field
            };
            Uint32 cmp_val;         // Value for comparation
        };

        // Set flag to indicate this var uninitialized
        void set_uninitialized()
        {
            version_no = VERSION_UNINITIALIZED;
        }

        // Set flag to indicate this var partial initialized
        void set_partial_initialized()
        {
            version_no = VERSION_PARTIAL_INITIALIZED;
        }

        // set_var_name
        VarInfo(VarStorage _storage, VariableNo _var_no) :
            version_no(0),
            storage(_storage),
            var_no(_var_no),
            unused(0)
        {
        }
    };
    typedef simple::vector<VarInfo> VarInfos;

    // Phi node
    struct Incoming
    {
        VersionNo version_no;   // Version of variable
        BasicBlockId id;        // From block

        Incoming(BasicBlockId _id, VersionNo _version_no = 0) :
            id(_id),
            version_no(_version_no)
        {
        }
    };
    struct PhiNode
    {
        VarInfo var_info;   // Variable & output version
        simple::vector<Incoming> incoming; // From blocks & the version

        PhiNode(const VarInfo& _var_info) :
            var_info(_var_info)
        {
        }
    };
    typedef simple::vector<PhiNode> PhiNodes;

    // Predicate function for VarInfo
    struct VarInfoLess
    {
        bool operator ()(const VarInfo& a, const VarInfo& b)
        {
            return (a.cmp_val < b.cmp_val);
        }
    };
    struct VarInfoEuqalTo
    {
        bool operator ()(const VarInfo& a, const VarInfo& b)
        {
            return (a.cmp_val == b.cmp_val);
        }
    };
    struct PhiNodeVarInfoLess
    {
        bool operator ()(const PhiNode& a, const PhiNode& b)
        {
            return VarInfoLess()(a.var_info, b.var_info);
        }

        bool operator ()(const PhiNode& a, const VarInfo& b)
        {
            return VarInfoLess()(a.var_info, b);
        }

        bool operator ()(const VarInfo& a, const PhiNode& b)
        {
            return VarInfoLess()(a, b.var_info);
        }
    };
    struct PhiNodeVarInfoEuqalTo
    {
        bool operator ()(const PhiNode& a, const PhiNode& b)
        {
            return VarInfoEuqalTo()(a.var_info, b.var_info);
        }
    };

public:
    BasicBlockId     id;        // My ID
    BasicBlockAttrib attrib;    // Attrib of this block
    AstNodeNo        begin;     // Begin offset of ast nodes in m_nodes
    AstNodeNo        count;     // nodes in this block
    BasicBlockId     idom;      // My immediate dominator, the closest dominator
    BasicBlockIds    preds;     // Predecessors blocks of me
    BasicBlockIds    branches;  // Branch to other blocks
    BasicBlockIds    df;        // Dominance Frontiers

public:
    PhiNodes         phi_nodes; // Phi nodes
    VarInfos         inputs;    // Variables used by this block
    VarInfos         outputs;   // Variables outputed by this block

    bool is_not_reachable()
    {
        return (attrib & BASIC_BLOCK_NOT_REACHABLE) ? true : false;
    }
};
typedef simple::vector<BasicBlock*> BasicBlocks;

}