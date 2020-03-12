/*

Kaitshs 4coder custom layer
 
heavily inspired by Ryan Fleurys custom
layer (https://github.com/ryanfleury/4coder_fleury)

*/
//#if !defined(FCODER_DEFAULT_BINDINGS_CPP)

#include "4coder_default_include.cpp"
#include "generated/managed_id_metadata.cpp"

#include "4coder_kaitsh/4coder_kaitsh_mapping.cpp"

// NOTE(dgl): Custom hook definitions

void
custom_layer_init(Application_Links *app)
{
    Thread_Context *tctx = get_thread_context(app);
    
    default_framework_init(app);
    set_all_default_hooks(app);
    
    mapping_init(tctx, &framework_mapping);
    KaitshSetCustomMapping(&framework_mapping);
}

//#define FCODER_DEFAULT_BINDINGS_CPP
//#endif
