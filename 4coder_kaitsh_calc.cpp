// TODO(dgl): Do bondary check on node buffer and operatorstack

enum CalcTokenType
{
    CALC_TOKEN_TYPE_Invalid,
    CALC_TOKEN_TYPE_Number,
    CALC_TOKEN_TYPE_String,
    CALC_TOKEN_TYPE_Plus,
    CALC_TOKEN_TYPE_Minus,
    CALC_TOKEN_TYPE_Asterisk,
    CALC_TOKEN_TYPE_Slash,
    CALC_TOKEN_TYPE_Percent,
    CALC_TOKEN_TYPE_Circumflex,
    CALC_TOKEN_TYPE_OpenParen,
    CALC_TOKEN_TYPE_CloseParen,
    CALC_TOKEN_TYPE_NewLine
};

// NOTE(dgl): Cool trick from Ryan Fleury (https://github.com/ryanfleury) to define enum and a corresponding value at the same place. First we create a list containing a preprocessor functions which take the values. Then we create an enum with the types. In the enum we define the preprocessor function used in the list and we call the list. And undefine the function, so we can redefine it. For the other value we create a function with a static table. To create this table we define the preprocessor function again, call the list and undefine the funciton again.
#define CALC_NODE_TYPE_LIST                \
CALC_NODE_TYPE(Error,                  0)\
CALC_NODE_TYPE(Constant,               0)\
CALC_NODE_TYPE(OpenParen,              0)\
CALC_NODE_TYPE(CloseParen,              0)\
CALC_NODE_TYPE(Add,                    1)\
CALC_NODE_TYPE(Subtract,               1)\
CALC_NODE_TYPE(Multiply,               2)\
CALC_NODE_TYPE(Divide,                 2)\
CALC_NODE_TYPE(Modulus,                2)\
CALC_NODE_TYPE(RaiseToPower,           3)

enum CalcNodeType
{
#define CALC_NODE_TYPE(name, precedence) CALC_NODE_TYPE_##name,
    CALC_NODE_TYPE_LIST
#undef CALC_NODE_TYPE
};

static int
CalcOperatorPrecedence(CalcNodeType type)
{
    local_persist int precedence_table[] =
    {
#define CALC_NODE_TYPE(name, precedence) precedence,
        CALC_NODE_TYPE_LIST
#undef CALC_NODE_TYPE
    };
    
    return precedence_table[type];
}

struct CalcToken
{
    CalcTokenType Type;
    union
    {
        u8 *Data;
        char *String;
    };
    int Length;
};

struct CalcNode
{
    CalcNodeType Type;
    double Value;
    CalcNode *Next;
};

struct CalcMemory
{
    u32 Size;
    void* Buffer;
    
    CalcNode *NodeBufferPtr;
    CalcNode **OperatorStackPtr;
};

struct CalcTokenizer
{
    u8 *At;
};

b32 IsNumeric(char c)
{
    return(('0' <= c) && (c <= '9'));
}

b32 IsAlpha(char c)
{
    return(('a' <= c) && (c <= 'z') ||
           ('A' <= c) && (c <= 'Z'));
}

internal CalcToken
GetToken(CalcTokenizer *Tokenizer)
{
    CalcToken Token = {};
    Token.Type = CALC_TOKEN_TYPE_Invalid;
    Token.Data = Tokenizer->At;
    Token.Length = 1;
    
    char C = *Tokenizer->At;
    ++Tokenizer->At;
    switch(C)
    {
        case '+': { Token.Type = CALC_TOKEN_TYPE_Plus; } break;
        case '-': { Token.Type = CALC_TOKEN_TYPE_Minus; } break;
        case '*': { Token.Type = CALC_TOKEN_TYPE_Asterisk; } break;
        case '/': { Token.Type = CALC_TOKEN_TYPE_Slash; } break;
        case '%': { Token.Type = CALC_TOKEN_TYPE_Percent; } break;
        case '^': { Token.Type = CALC_TOKEN_TYPE_Circumflex; } break;
        case '(': { Token.Type = CALC_TOKEN_TYPE_OpenParen; } break;
        case ')': { Token.Type = CALC_TOKEN_TYPE_CloseParen; } break;
        case '\n': { Token.Type = CALC_TOKEN_TYPE_NewLine; } break;
        default:
        {
            if(IsNumeric(C))
            {
                Token.Type = CALC_TOKEN_TYPE_Number;
                while(IsNumeric(*Tokenizer->At) ||
                      (*Tokenizer->At == '.') ||
                      (*Tokenizer->At == 'f'))
                {
                    ++Tokenizer->At;
                    Token.Length = Tokenizer->At - Token.Data;
                }
            }
            else if(IsAlpha(C))
            {
                Token.Type = CALC_TOKEN_TYPE_String;
                while(IsAlpha(*Tokenizer->At))
                {
                    ++Tokenizer->At;
                    Token.Length = Tokenizer->At - Token.Data;
                }
            }
            else
            {
                Token.Type = CALC_TOKEN_TYPE_Invalid;
            }
        } break;
    }
    return(Token);
}

internal CalcToken
PeekToken(CalcTokenizer *Tokenizer)
{
    CalcTokenizer Tokenizer2 = *Tokenizer;
    CalcToken Result = GetToken(&Tokenizer2);
    return(Result);
}

internal CalcNode
ParseNode(CalcTokenizer *Tokenizer, CalcToken *Token)
{
    CalcNode Result = {};
    Result.Type = CALC_NODE_TYPE_Error;
    
    switch(Token->Type)
    {
        case CALC_TOKEN_TYPE_OpenParen: { Result.Type = CALC_NODE_TYPE_OpenParen; } break;
        case CALC_TOKEN_TYPE_CloseParen: { Result.Type = CALC_NODE_TYPE_CloseParen; } break;
        case CALC_TOKEN_TYPE_Plus: { Result.Type = CALC_NODE_TYPE_Add; } break;
        case CALC_TOKEN_TYPE_Minus: { Result.Type = CALC_NODE_TYPE_Subtract; } break;
        case CALC_TOKEN_TYPE_Asterisk:
        {
            CalcToken Peeked = PeekToken(Tokenizer);
            if(Peeked.Type == CALC_TOKEN_TYPE_Slash)
            {
                Result.Type = CALC_NODE_TYPE_Error;
                GetToken(Tokenizer);
            }
            else
            {
                Result.Type = CALC_NODE_TYPE_Multiply;
            }
        } break;
        case CALC_TOKEN_TYPE_Slash:
        {
            CalcToken Peeked = PeekToken(Tokenizer);
            if((Peeked.Type == CALC_TOKEN_TYPE_Slash) ||
               (Peeked.Type == CALC_TOKEN_TYPE_Asterisk))
            {
                Result.Type = CALC_NODE_TYPE_Error;
                GetToken(Tokenizer);
            }
            else
            {
                Result.Type = CALC_NODE_TYPE_Divide;
            }
        } break;
        case CALC_TOKEN_TYPE_Percent: { Result.Type = CALC_NODE_TYPE_Modulus; } break;
        case CALC_TOKEN_TYPE_Circumflex: { Result.Type = CALC_NODE_TYPE_RaiseToPower; } break;
        case CALC_TOKEN_TYPE_Number:
        {
            Result.Type = CALC_NODE_TYPE_Constant;
            char *StringEnd = Token->String + (Token->Length - 1);
            Result.Value = strtod(Token->String, &StringEnd);
        } break;
        case CALC_TOKEN_TYPE_String:
        {
            String_Const_char TokenText = {};
            TokenText.str = Token->String;
            TokenText.size = Token->Length;
            if((string_compare(TokenText, string_litexpr("pi")) == 0) ||
               (string_compare(TokenText, string_litexpr("PI")) == 0))
            {
                Result.Type = CALC_NODE_TYPE_Constant;
                Result.Value = 3.1415926535897932;
            }
            else if((string_compare(TokenText, string_litexpr("e")) == 0) ||
                    (string_compare(TokenText, string_litexpr("E")) == 0))
            {
                Result.Type = CALC_NODE_TYPE_Constant;
                Result.Value = 2.7182818284590452;
            }
            else
            {
                Result.Type = CALC_NODE_TYPE_Error;
            }
        } break;
        default: { Result.Type = CALC_NODE_TYPE_Error; } break;
    }
    
    return(Result);
}


internal int
PushOperator(CalcMemory *Memory, CalcNode *Node)
{
    int Result = -1;
    if(((CalcNode **)Memory->NodeBufferPtr < (Memory->OperatorStackPtr - 1)) &&
       (Memory->OperatorStackPtr <= (CalcNode **)((u8 *)Memory->Buffer + Memory->Size)))
    {
        *Memory->OperatorStackPtr = Node;
        Memory->OperatorStackPtr--;
        Result = 0;
    }
    return Result;
}

internal CalcNode *
PeekOperator(CalcMemory *Memory)
{
    CalcNode *Result = 0;
    
    if(Memory->OperatorStackPtr < (CalcNode **)((u8 *)Memory->Buffer + Memory->Size))
    {
        Result = *(Memory->OperatorStackPtr + 1);
    }
    
    return(Result);
}

internal CalcNode *
PopOperator(CalcMemory *Memory)
{
    CalcNode *Result = PeekOperator(Memory);
    if(Result)
    {
        Memory->OperatorStackPtr++;
        Assert(Memory->OperatorStackPtr <= (CalcNode **)((u8 *)Memory->Buffer + Memory->Size));
    }
    
    return(Result);
}

internal b32
OperatorStackEmpty(CalcMemory *Memory)
{
    return(Memory->OperatorStackPtr >= (CalcNode **)((u8 *)Memory->Buffer + Memory->Size));
}

internal CalcNode *
ParseLineToPostfix(CalcTokenizer *Tokenizer, CalcMemory *Memory, void *Buffer, u64 Size)
{
    // NOTE(dgl): Temp starting point of the list, because we only assign nodes to the "Next" field of the stucture.
    CalcNode TmpRoot = {};
    CalcNode *Last = &TmpRoot;
    b32 ParsingError = false;
    
    CalcToken Token;
    while(!ParsingError && Tokenizer->At < ((u8 *)Buffer + Size))
    {
        Token = GetToken(Tokenizer);
        if(Token.Type == CALC_TOKEN_TYPE_NewLine)
        {
            ParsingError = true;
        }
        
        CalcNode Parsed = ParseNode(Tokenizer, &Token);
        if(Parsed.Type == CALC_NODE_TYPE_Error)
        {
            ParsingError = true;
        }
        
        CalcNode *Node;
        if(Memory->NodeBufferPtr < (CalcNode *)Memory->OperatorStackPtr)
        {
            Node = Memory->NodeBufferPtr++;
            Node->Type = Parsed.Type;
            Node->Value = Parsed.Value;
            Node->Next = 0;
        }
        else
        {
            ParsingError = true;
        }
        
        if(!ParsingError)
        {
            switch(Node->Type)
            {
                case CALC_NODE_TYPE_OpenParen:
                {
                    if(PushOperator(Memory, Node) < 0)
                    {
                        ParsingError = true;
                    }
                } break;
                case CALC_NODE_TYPE_CloseParen:
                {
                    CalcNode *Next;
                    while((Next = PopOperator(Memory)) &&
                          (Next->Type != CALC_NODE_TYPE_OpenParen))
                    {
                        Last->Next = Next;
                        Last = Next;
                    }
                    
                } break;
                case CALC_NODE_TYPE_Add:
                case CALC_NODE_TYPE_Subtract:
                case CALC_NODE_TYPE_Multiply:
                case CALC_NODE_TYPE_Divide:
                case CALC_NODE_TYPE_Modulus:
                case CALC_NODE_TYPE_RaiseToPower:
                {
                    while(!OperatorStackEmpty(Memory) && (CalcOperatorPrecedence(PeekOperator(Memory)->Type) >= CalcOperatorPrecedence(Node->Type)))
                    {
                        CalcNode *Next = PopOperator(Memory);
                        Last->Next = Next;
                        Last = Next;
                    }
                    if(PushOperator(Memory, Node) < 0)
                    {
                        ParsingError = true;
                    }
                } break;
                case CALC_NODE_TYPE_Constant:
                {
                    Last->Next = Node;
                    Last = Node;
                } break;
                default:
                {
                    // TODO(dgl): ignore everything else
                } break;
            }
        }
        
        // NOTE(dgl): Empty operator stack
        while(!OperatorStackEmpty(Memory))
        {
            CalcNode *Next = PopOperator(Memory);
            Last->Next = Next;
            Last = Next;
        }
        Assert(Memory->OperatorStackPtr == (CalcNode **)((u8 *)Memory->Buffer + Memory->Size));
    }
    
    // NOTE(dgl): The first "real" node is the one after root
    return(TmpRoot.Next);
}

internal double
EvaluatePostfix(CalcMemory *Memory, CalcNode *Start)
{
    CalcNode *Node = Start;
    b32 IsValid = true;
    while(IsValid && (Node != 0))
    {
        // TODO(dgl): Compress calculation
        switch(Node->Type)
        {
            case CALC_NODE_TYPE_Constant: { PushOperator(Memory, Node); } break;
            case CALC_NODE_TYPE_Add:
            {
                CalcNode *Right = PopOperator(Memory);
                CalcNode *Left = PopOperator(Memory);
                
                if(Right && Left)
                {
                    // NOTE(dgl): The node we store the result does not matter
                    Left->Value = Left->Value + Right->Value;
                    PushOperator(Memory, Left);
                }
                else
                {
                    IsValid = false;
                }
            } break;
            case CALC_NODE_TYPE_Subtract:
            {
                CalcNode *Right = PopOperator(Memory);
                CalcNode *Left = PopOperator(Memory);
                
                if(Right && Left)
                {
                    Left->Value = Left->Value - Right->Value;
                    PushOperator(Memory, Left);
                }
                else
                {
                    IsValid = false;
                }
            } break;
            case CALC_NODE_TYPE_Multiply:
            {
                CalcNode *Right = PopOperator(Memory);
                CalcNode *Left = PopOperator(Memory);
                
                if(Right && Left)
                {
                    Left->Value = Left->Value * Right->Value;
                    PushOperator(Memory, Left);
                }
                else
                {
                    IsValid = false;
                }
            } break;
            case CALC_NODE_TYPE_Divide:
            {
                CalcNode *Right = PopOperator(Memory);
                CalcNode *Left = PopOperator(Memory);
                
                if(Right && Left && (Right->Value != 0))
                {
                    Left->Value = Left->Value / Right->Value;
                    PushOperator(Memory, Left);
                }
                else
                {
                    IsValid = false;
                }
            } break;
            case CALC_NODE_TYPE_Modulus:
            {
                CalcNode *Right = PopOperator(Memory);
                CalcNode *Left = PopOperator(Memory);
                
                if(Right && Left && (Right->Value != 0))
                {
                    Left->Value = fmod(Left->Value, Right->Value);
                    PushOperator(Memory, Left);
                }
                else
                {
                    IsValid = false;
                }
            } break;
            case CALC_NODE_TYPE_RaiseToPower:
            {
                CalcNode *Right = PopOperator(Memory);
                CalcNode *Left = PopOperator(Memory);
                
                if(Right && Left)
                {
                    Left->Value = pow(Left->Value, Right->Value);
                    PushOperator(Memory, Left);
                }
                else
                {
                    IsValid = false;
                }
            } break;
            default:
            {
                // TODO(dgl): should not happen (logging)
            } break;
        }
        
        if(Node->Next)
        {
            Node = Node->Next;
        }
        else
        {
            Node = 0;
        }
    }
    
    double Result = NAN;
    if(IsValid && !OperatorStackEmpty(Memory))
    {
        Node = PopOperator(Memory);
        Result = Node->Value;
    }
    
    return(Result);
}

internal void
RenderCommentCode(Application_Links *app, Buffer_ID buffer, Text_Layout_ID text_layout_id, i64 Position, String_Const_u8 TextBuffer)
{
    CalcTokenizer Tokenizer = {};
    Tokenizer.At = TextBuffer.str;
    
    // TODO(dgl): I don't like this solution. Maybe I have a better solution in the future.
    if(Tokenizer.At[2] == 'c')
    {
        // TODO(dgl): Segfault when starting calcualtion with (
        // TODO(dgl): Stacksmashing when stack not large enough
        // In theory we should not store the nodes, if there is not enough memory
        u8 MemoryBuffer[2*1024*1024];
        CalcMemory Memory = {};
        Memory.Buffer = (void *)MemoryBuffer;
        Memory.Size = ArrayCount(MemoryBuffer);
        Memory.NodeBufferPtr = (CalcNode *)Memory.Buffer;
        
        while(Tokenizer.At < (TextBuffer.str + TextBuffer.size))
        {
            // NOTE(dgl): Reset OperatorStackPtr for each calculation
            Memory.OperatorStackPtr = (CalcNode **)((u8 *)Memory.Buffer + Memory.Size);
            CalcNode *Root = ParseLineToPostfix(&Tokenizer, &Memory, TextBuffer.data, TextBuffer.size);
            
            if(Root)
            {
                // TODO(dgl): Support calculating with variables or functions
                double Result = EvaluatePostfix(&Memory, Root);
                
                char ResultBuffer[256];
                String_Const_u8 ResultString = {(u8 *)ResultBuffer};
                ResultString.size = sprintf(ResultBuffer, "=> %f", Result);
                
                Vec2_f32 ResultPosition = {0};
                
                u64 PositionOffset = Tokenizer.At - 1 - TextBuffer.str;
                u64 LastCharacterPosition = Position + PositionOffset;
                Rect_f32 LastCharacterRect = text_layout_character_on_screen(app, text_layout_id, LastCharacterPosition);
                
                ResultPosition.x = LastCharacterRect.x0;
                ResultPosition.y = LastCharacterRect.y0;
                ResultPosition.x += 15;
                
                ARGB_Color Color = finalize_color(defcolor_comment, 1);
                draw_string(app, get_face_id(app, buffer), ResultString, ResultPosition, Color);
            }
        }
    }
    else
    {
        // TODO(dgl): Do nothing if it is not a calc comment
    }
}

internal void
KaitshRenderCommentCalc(Application_Links *app, Buffer_ID buffer, Text_Layout_ID text_layout_id)
{
    // NOTE(dgl): similar to draw_comment_highlights
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    if (token_array.tokens != 0)
    {
        Scratch_Block scratch(app);
        
        Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
        i64 first_index = token_index_from_pos(&token_array, visible_range.first);
        Token_Iterator_Array it = token_iterator_index(buffer, &token_array, first_index);
        
        while(true)
        {
            Temp_Memory_Block temp(scratch);
            Token *token = token_it_read(&it);
            if(token->pos >= visible_range.one_past_last)
            {
                break;
            }
            String_Const_u8 tail = {};
            if(token_it_check_and_get_lexeme(app, scratch, &it, TokenBaseKind_Comment, &tail))
            {
                // TODO(dgl): Render only Calc Comment
                RenderCommentCode(app, buffer, text_layout_id, token->pos, tail);
            }
            if(!token_it_inc_non_whitespace(&it))
            {
                break;
            }
        }
    }
}
