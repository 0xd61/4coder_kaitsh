/*
Functions for loading project locations from file
*/

CUSTOM_COMMAND_SIG(close_all_files)
CUSTOM_DOC("close all files that are opened no matter the extension")
{
	Buffer_ID buffer = get_buffer_next(app, 0, Access_Always);
	while (buffer != 0)
	{
		buffer_kill(app, buffer, BufferKill_AlwaysKill);
		buffer = get_buffer_next(app, buffer, Access_Always);
	}
}

CUSTOM_UI_COMMAND_SIG(project_lister)
CUSTOM_DOC("List current projects and jump and load to one chosen by the user.")
{
    Scratch_Block scratch(app);
    
    // NOTE(dgl): We currently support up to 20 projects;
    String_Const_u8 Projects[20];
    uint ProjectCount = 0;
    b32 success = false;
    FILE *FileHandle = open_file_try_current_path_then_binary_path(app, "projects.4coder");
    if (FileHandle != 0)
    {
        Data FileContent = dump_file_handle(scratch, FileHandle);
        fclose(FileHandle);
        if (FileContent.data != 0)
        {
            String_Const_u8 *LatestProject = &Projects[ProjectCount++];
            u8 *Source = FileContent.data;
            LatestProject->str = FileContent.data;
            while(Source < (FileContent.data + FileContent.size))
            {
                if(*Source == '\n' && ProjectCount <= ArrayCount(Projects))
                {
                    LatestProject->size = Source - LatestProject->str;
                    printf("%s, %d\n", LatestProject->str, LatestProject->size);
                    
                    // Switch to new project string
                    Source++;
                    LatestProject = &Projects[ProjectCount++];
                    LatestProject->str = Source;
                }
                else
                {
                    Source++;
                }
            }
            success = true;
        }
    }
    
    // TODO(dgl): Remove empty strings
    ProjectCount--;
    
    if (success){
        char *query = "Project:";
        
        Lister_Block lister(app, scratch);
        lister_set_query(lister, query);
        lister_set_default_handlers(lister);
        
        code_index_lock();
        
        for(int Index = 0; Index < ProjectCount; ++Index)
        {
            String_Const_u8 Name = {};
            lister_add_item(lister, Projects[Index], Name, &Projects[Index], 0);
            printf("%s\n", Projects[Index].str);
        }
        
        code_index_unlock();
        
        printf("HERE!!\n");
        Lister_Result l_result = run_lister(app, lister);
        String_Const_u8 *result = 0;
        
        if(!l_result.canceled && l_result.user_data != 0)
        {
            result = (String_Const_u8 *)l_result.user_data;
            if (result->str != 0){
                set_hot_directory(app, *result);
                close_all_files(app);
                load_project(app);
            }
        }
    }
    /* String_Const_u8 Projects[] = {
        string_u8_litexpr("/home/danielg/Sync/4coder/"),
        string_u8_litexpr("/home/danielg/Code/C/handmade_hero/"),
        string_u8_litexpr("/home/danielg/Code/Elixir/ex_consulta_saldo/")
    }; */
    
}