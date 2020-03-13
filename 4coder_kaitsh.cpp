/*

Kaitshs 4coder custom layer
  
// TODO(dgl):
- [ ] Project list and quick select
- [ ] Smooth cursor (fleury)
- [ ] Elixir Language

*/

#if !defined(FCODER_DEFAULT_BINDINGS_CPP)

#include "4coder_default_include.cpp"
#include "generated/managed_id_metadata.cpp"

#include "4coder_kaitsh_projects.cpp"
#include "4coder_kaitsh_mapping.cpp"

#define global static
#define internal static
#define local_persist static

internal void
KaitshRenderCaller(Application_Links *app, Frame_Info frame_info, View_ID view_id)
{
    
}

void
custom_layer_init(Application_Links *app)
{
    Thread_Context *tctx = get_thread_context(app);
    
    default_framework_init(app);
    set_all_default_hooks(app);
    //set_custom_hook(app, HookID_RenderCaller,  KaitshRenderCaller);
    
    mapping_init(tctx, &framework_mapping);
    KaitshSetCustomMapping(&framework_mapping, mapid_global, mapid_file, mapid_code);
}

#define FCODER_DEFAULT_BINDINGS_CPP
#endif
