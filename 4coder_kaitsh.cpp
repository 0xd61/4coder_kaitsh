/*

  Kaitshs 4coder custom layer

// TODO(dgl):
- [X] Project list and quick select
- [X] Smooth cursor (fleury)
- [X] Calc in Comments
- [ ] Calulate/Render only once
- [ ] Elixir Language

*/

#if !defined(FCODER_DEFAULT_BINDINGS_CPP)
#define FCODER_DEFAULT_BINDINGS_CPP

#include "4coder_default_include.cpp"
#include "generated/managed_id_metadata.cpp"

#include "4coder_kaitsh_types.cpp"
#include "4coder_kaitsh_projects.cpp"
#include "4coder_kaitsh_calc.cpp"
#include "4coder_kaitsh_cursor.cpp"
#include "4coder_kaitsh_mapping.cpp"

CUSTOM_COMMAND_SIG(kaitsh_toggle_battery_saver)
CUSTOM_DOC("Toggle the battery saver global variable.")
{
    global_battery_saver = !global_battery_saver;
}

CUSTOM_COMMAND_SIG(kaitsh_cut_line)
CUSTOM_DOC("Cut the line the on which the cursor sits.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    i64 pos = view_get_cursor_pos(app, view);
    i64 line = get_line_number_from_pos(app, buffer, pos);
    Range_i64 range = get_line_pos_range(app, buffer, line);
    
    if (clipboard_post_buffer_range(app, 0, buffer, range)){
        range.end += 1;
        buffer_replace_range(app, buffer, range, string_u8_empty);
    }
}

CUSTOM_UI_COMMAND_SIG(kaitsh_jump_to_definition_of_identifier)
CUSTOM_DOC("Jump to the definition of the keyword under the cursor.")
{
    char *query = "Definition:";
    
    Scratch_Block scratch(app);
    Lister_Block lister(app, scratch);
    String_Const_u8 needle = push_token_or_word_under_active_cursor(app, scratch);
    lister_set_query(lister, query);
    lister_set_key(lister, needle);
    lister_set_text_field(lister, needle);
    lister_set_default_handlers(lister);
    
    
    code_index_lock();
    for (Buffer_ID buffer = get_buffer_next(app, 0, Access_Always);
         buffer != 0;
         buffer = get_buffer_next(app, buffer, Access_Always)){
        Code_Index_File *file = code_index_get_file(buffer);
        if (file != 0){
            for (i32 i = 0; i < file->note_array.count; i += 1){
                Code_Index_Note *note = file->note_array.ptrs[i];
                
                Tiny_Jump *jump = push_array(scratch, Tiny_Jump, 1);
                jump->buffer = buffer;
                jump->pos = note->pos.first;
                
                String_Const_u8 sort = {};
                switch (note->note_kind){
                    case CodeIndexNote_Type:
                    {
                        sort = string_u8_litexpr("type");
                    }break;
                    case CodeIndexNote_Function:
                    {
                        sort = string_u8_litexpr("function");
                    }break;
                    case CodeIndexNote_Macro:
                    {
                        sort = string_u8_litexpr("macro");
                    }break;
                }
                lister_add_item(lister, note->text, sort, jump, 0);
            }
        }
    }
    code_index_unlock();
    
    Lister_Result l_result = run_lister(app, lister);
    Tiny_Jump result = {};
    if (!l_result.canceled && l_result.user_data != 0){
        block_copy_struct(&result, (Tiny_Jump*)l_result.user_data);
    }
    
    if (result.buffer != 0){
        View_ID view = get_this_ctx_view(app, Access_Always);
        jump_to_location(app, view, result.buffer, result.pos);
    }
}

internal void
KaitshDrawFileBar(Application_Links *app, View_ID view_id, Buffer_ID buffer, Face_ID face_id, Rect_f32 bar)
{
    Scratch_Block scratch(app);
    
    draw_rectangle_fcolor(app, bar, 0.f, fcolor_id(defcolor_bar));
    
    FColor base_color = fcolor_id(defcolor_base);
    FColor pop2_color = fcolor_id(defcolor_pop2);
    
    i64 cursor_position = view_get_cursor_pos(app, view_id);
    Buffer_Cursor cursor = view_compute_cursor(app, view_id, seek_pos(cursor_position));
    i64 LineCount = buffer_get_line_count(app, buffer);
    i64 BufferPercent = (i64)(((f32)cursor.line / (f32)LineCount) * 100);
    
    Fancy_Line list = {};
    String_Const_u8 unique_name = push_buffer_unique_name(app, scratch, buffer);
    push_fancy_string(scratch, &list, base_color, unique_name);
    push_fancy_stringf(scratch, &list, base_color, " - Row: %3.lld Col: %3.lld (%2lld%) -", cursor.line, cursor.col, BufferPercent);
    
    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    Line_Ending_Kind *eol_setting = scope_attachment(app, scope, buffer_eol_setting,
                                                     Line_Ending_Kind);
    switch (*eol_setting){
        case LineEndingKind_Binary:
        {
            push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" bin"));
        }break;
        
        case LineEndingKind_LF:
        {
            push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" lf"));
        }break;
        
        case LineEndingKind_CRLF:
        {
            push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" crlf"));
        }break;
    }
    
    u8 space[3];
    {
        Dirty_State dirty = buffer_get_dirty_state(app, buffer);
        String_u8 str = Su8(space, 0, 3);
        if (dirty != 0){
            string_append(&str, string_u8_litexpr(" "));
        }
        if (HasFlag(dirty, DirtyState_UnsavedChanges)){
            string_append(&str, string_u8_litexpr("*"));
        }
        if (HasFlag(dirty, DirtyState_UnloadedChanges)){
            string_append(&str, string_u8_litexpr("!"));
        }
        push_fancy_string(scratch, &list, pop2_color, str.string);
    }
    
    /*if(global_keyboard_macro_is_recording)
    {
        Rect_f32 recording_marker;
        recording_marker.x0 = bar.x1 - 20.f;
        recording_marker.x1 = recording_marker.x0 + 10.f;
        recording_marker.y0 = bar.y0 + ((bar.y1 - bar.y0) / 2) - 5.f;
        recording_marker.y1 = recording_marker.y0 + 10.f;
        draw_rectangle_fcolor(app, recording_marker, 5.f, pop2_color);
    }*/
    
    Vec2_f32 p = bar.p0 + V2f32(2.f, 2.f);
    draw_fancy_line(app, face_id, fcolor_zero(), &list, p);
}

internal b32
KaitshStringMatchPrefix(String_Const_u8 String, String_Const_u8 Input)
{
    String_Const_u8 Prefix = string_prefix(String, Input.size);
    return(string_match(Prefix, Input));
}

internal void
KaitshDrawCommentHighlights(Application_Links *app, Buffer_ID buffer, Text_Layout_ID text_layout_id,
                            Token_Array *array, String_Const_u8 prefix, ARGB_Color color)
{
    Scratch_Block scratch(app);
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    i64 first_index = token_index_from_pos(array, visible_range.first);
    Token_Iterator_Array it = token_iterator_index(buffer, array, first_index);
    for (;;)
    {
        Temp_Memory_Block temp(scratch);
        Token *token = token_it_read(&it);
        if (token->pos >= visible_range.one_past_last)
        {
            break;
        }
        String_Const_u8 tail = {};
        if (token_it_check_and_get_lexeme(app, scratch, &it, TokenBaseKind_Comment, &tail))
        {
            Range_i64 highlight = {-1, -1};
            i64 index = 0;
            for (index = token->pos;
                 tail.size > 0;
                 tail = string_skip(tail, 1), index += 1)
            {
                if (KaitshStringMatchPrefix(tail, prefix))
                {
                    highlight.start = index;
                    highlight.end = -1;
                    index += prefix.size -1;
                    tail = string_skip(tail, prefix.size - 1);
                    
                    while((tail.size > 0) && (!KaitshStringMatchPrefix(tail, string_u8_litexpr(" ")) && !KaitshStringMatchPrefix(tail, string_u8_litexpr("\n")) &&
                                              !KaitshStringMatchPrefix(tail, string_u8_litexpr("*/"))))
                    {
                        tail = string_skip(tail, 1);
                        index += 1;
                    }
                    
                    highlight.end = index;
                    paint_text_color(app, text_layout_id, highlight, color);
                }
            }
            if(highlight.start >= 0 && highlight.end < 0)
            {
                highlight.end = index - 1;
                paint_text_color(app, text_layout_id, highlight, color);
            }
            
        }
        if (!token_it_inc_non_whitespace(&it))
        {
            break;
        }
    }
}

internal void
KaitshRenderBuffer(Application_Links *app, View_ID view_id, Face_ID face_id,
                   Buffer_ID buffer, Text_Layout_ID text_layout_id,
                   Rect_f32 rect, Frame_Info frame_info)
{
    ProfileScope(app, "[kaitsh] render buffer");
    
    View_ID active_view = get_active_view(app, Access_Always);
    b32 is_active_view = (active_view == view_id);
    Rect_f32 prev_clip = draw_set_clip(app, rect);
    
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    
    // NOTE(allen): Cursor shape
    Face_Metrics metrics = get_face_metrics(app, face_id);
    f32 cursor_roundness = metrics.normal_advance*global_config.cursor_roundness;
    f32 mark_thickness = (f32)global_config.mark_thickness;
    
    // NOTE(allen): Token colorizing
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    if (token_array.tokens != 0){
        draw_cpp_token_colors(app, text_layout_id, &token_array);
        
        // NOTE(allen): Scan for TODOs and NOTEs
        if (global_config.use_comment_keyword){
            Comment_Highlight_Pair pairs[] = {
                {string_u8_litexpr("NOTE"), finalize_color(defcolor_comment_pop, 0)},
                {string_u8_litexpr("TODO"), finalize_color(defcolor_comment_pop, 1)},
            };
            draw_comment_highlights(app, buffer, text_layout_id,
                                    &token_array, pairs, ArrayCount(pairs));
            
            // NOTE(dgl): Highlight words prefixed with @@ in comments
            KaitshDrawCommentHighlights(app, buffer, text_layout_id, &token_array, string_u8_litexpr("@@"), finalize_color(defcolor_comment_pop, 2));
        }
        
    }
    else{
        paint_text_color_fcolor(app, text_layout_id, visible_range, fcolor_id(defcolor_text_default));
    }
    
    i64 cursor_pos = view_correct_cursor(app, view_id);
    view_correct_mark(app, view_id);
    
    // NOTE(allen): Scope highlight
    if (global_config.use_scope_highlight){
        Color_Array colors = finalize_color_array(defcolor_back_cycle);
        draw_scope_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
    }
    
    if (global_config.use_error_highlight || global_config.use_jump_highlight){
        // NOTE(allen): Error highlight
        String_Const_u8 name = string_u8_litexpr("*compilation*");
        Buffer_ID compilation_buffer = get_buffer_by_name(app, name, Access_Always);
        if (global_config.use_error_highlight){
            draw_jump_highlights(app, buffer, text_layout_id, compilation_buffer,
                                 fcolor_id(defcolor_highlight_junk));
        }
        
        // NOTE(allen): Search highlight
        if (global_config.use_jump_highlight){
            Buffer_ID jump_buffer = get_locked_jump_buffer(app);
            if (jump_buffer != compilation_buffer){
                draw_jump_highlights(app, buffer, text_layout_id, jump_buffer,
                                     fcolor_id(defcolor_highlight_white));
            }
        }
    }
    
    // NOTE(allen): Color parens
    if (global_config.use_paren_helper){
        Color_Array colors = finalize_color_array(defcolor_text_cycle);
        draw_paren_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
    }
    
    // NOTE(allen): Line highlight
    if (global_config.highlight_line_at_cursor && is_active_view){
        i64 line_number = get_line_number_from_pos(app, buffer, cursor_pos);
        draw_line_highlight(app, text_layout_id, line_number,
                            fcolor_id(defcolor_highlight_cursor_line));
    }
    
    // NOTE(allen): Whitespace highlight
    b64 show_whitespace = false;
    view_get_setting(app, view_id, ViewSetting_ShowWhitespace, &show_whitespace);
    if (show_whitespace){
        if (token_array.tokens == 0){
            draw_whitespace_highlight(app, buffer, text_layout_id, cursor_roundness);
        }
        else{
            draw_whitespace_highlight(app, text_layout_id, &token_array, cursor_roundness);
        }
    }
    
    // NOTE(allen): Cursor
    switch (fcoder_mode){
        case FCoderMode_Original:
        {
            // NOTE(dgl): Smooth cursor
            KaitshDrawCursorMarkHighlight(app, view_id, is_active_view, buffer, text_layout_id, cursor_roundness, mark_thickness, frame_info);
            // draw_original_4coder_style_cursor_mark_highlight(app, view_id, is_active_view, buffer, text_layout_id, cursor_roundness, mark_thickness);
        }break;
        case FCoderMode_NotepadLike:
        {
            draw_notepad_style_cursor_highlight(app, view_id, buffer, text_layout_id, cursor_roundness);
        }break;
    }
    
    // NOTE(allen): Fade ranges
    paint_fade_ranges(app, text_layout_id, buffer);
    
    // NOTE(allen): put the actual text on the actual screen
    draw_text_layout_default(app, text_layout_id);
    
    draw_set_clip(app, prev_clip);
    
    // NOTE(dgl): Draw calculations in comments
    KaitshRenderCommentCalc(app, buffer, text_layout_id);
}

internal void
KaitshRenderCaller(Application_Links *app, Frame_Info frame_info, View_ID view_id)
{
    ProfileScope(app, "[kaitsh] render caller");
    View_ID active_view = get_active_view(app, Access_Always);
    b32 is_active_view = (active_view == view_id);
    
    Rect_f32 region = draw_background_and_margin(app, view_id, is_active_view);
    Rect_f32 prev_clip = draw_set_clip(app, region);
    
    Buffer_ID buffer = view_get_buffer(app, view_id, Access_Always);
    Face_ID face_id = get_face_id(app, buffer);
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    f32 line_height = face_metrics.line_height;
    f32 digit_advance = face_metrics.decimal_digit_advance;
    
    // NOTE(allen): file bar
    b64 showing_file_bar = false;
    if (view_get_setting(app, view_id, ViewSetting_ShowFileBar, &showing_file_bar) && showing_file_bar){
        Rect_f32_Pair pair = layout_file_bar_on_top(region, line_height);
        KaitshDrawFileBar(app, view_id, buffer, face_id, pair.min);
        region = pair.max;
    }
    
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view_id);
    
    Buffer_Point_Delta_Result delta = delta_apply(app, view_id,
                                                  frame_info.animation_dt, scroll);
    if (!block_match_struct(&scroll.position, &delta.point)){
        block_copy_struct(&scroll.position, &delta.point);
        view_set_buffer_scroll(app, view_id, scroll, SetBufferScroll_NoCursorChange);
    }
    if (delta.still_animating){
        animate_in_n_milliseconds(app, 0);
    }
    
    // NOTE(allen): query bars
    region = default_draw_query_bars(app, region, view_id, face_id);
    
    // NOTE(allen): FPS hud
    if (show_fps_hud){
        Rect_f32_Pair pair = layout_fps_hud_on_bottom(region, line_height);
        draw_fps_hud(app, frame_info, face_id, pair.max);
        region = pair.min;
        animate_in_n_milliseconds(app, 1000);
    }
    
    // NOTE(allen): layout line numbers
    Rect_f32 line_number_rect = {};
    if (global_config.show_line_number_margins){
        Rect_f32_Pair pair = layout_line_number_margin(app, buffer, region, digit_advance);
        line_number_rect = pair.min;
        region = pair.max;
    }
    
    // NOTE(allen): begin buffer render
    Buffer_Point buffer_point = scroll.position;
    Text_Layout_ID text_layout_id = text_layout_create(app, buffer, region, buffer_point);
    
    // NOTE(allen): draw line numbers
    if (global_config.show_line_number_margins){
        draw_line_number_margin(app, view_id, buffer, face_id, text_layout_id, line_number_rect);
    }
    
    // NOTE(allen): draw the buffer
    KaitshRenderBuffer(app, view_id, face_id, buffer, text_layout_id, region, frame_info);
    
    text_layout_free(app, text_layout_id);
    draw_set_clip(app, prev_clip);
}

void
custom_layer_init(Application_Links *app)
{
    Thread_Context *tctx = get_thread_context(app);
    
    default_framework_init(app);
    set_all_default_hooks(app);
    set_custom_hook(app, HookID_RenderCaller,  KaitshRenderCaller);
    mapping_init(tctx, &framework_mapping);
#if OS_MAC
    setup_mac_mapping(&framework_mapping, mapid_global, mapid_file, mapid_code);
#else
    KaitshSetCustomMapping(&framework_mapping, mapid_global, mapid_file, mapid_code);
#endif
}

#endif