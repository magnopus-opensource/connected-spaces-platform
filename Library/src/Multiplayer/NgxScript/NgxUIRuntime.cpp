#include "Multiplayer/NgxScript/NgxUIRuntime.h"

#include "CSP/Common/Systems/Log/LogSystem.h"

#define CLAY_IMPLEMENTATION
#include "clay.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <deque>
#include <limits>
#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <string>
#include <vector>

namespace
{

enum class UISurfaceKind
{
    Screen,
    World,
};

enum class UILayoutKind
{
    Row,
    Column,
};

enum class UISizeMode
{
    Fit,
    Grow,
    Fixed,
};

enum class UIWidgetKind
{
    ScreenRoot,
    WorldRoot,
    Row,
    Column,
    FlowRow,
    Floating,
    Spacer,
    Text,
    Image,
    Button,
};

struct UISizeSpec
{
    UISizeMode Mode;
    float Value;
    float MinValue;
    float MaxValue;

    UISizeSpec()
        : Mode(UISizeMode::Fit)
        , Value(0.0f)
        , MinValue(0.0f)
        , MaxValue(0.0f)
    {
    }
};

struct UIPadding
{
    float Left;
    float Right;
    float Top;
    float Bottom;

    UIPadding()
        : Left(0.0f)
        , Right(0.0f)
        , Top(0.0f)
        , Bottom(0.0f)
    {
    }
};

struct UIColor
{
    float R;
    float G;
    float B;
    float A;

    UIColor()
        : R(0.0f)
        , G(0.0f)
        , B(0.0f)
        , A(0.0f)
    {
    }
};

UIColor MakeColor(float R, float G, float B, float A = 1.0f)
{
    UIColor Color;
    Color.R = R;
    Color.G = G;
    Color.B = B;
    Color.A = A;
    return Color;
}

struct UINode
{
    std::string Id;
    UIWidgetKind Kind;
    UISurfaceKind Surface;
    UILayoutKind Layout;
    UISizeSpec Width;
    UISizeSpec Height;
    UIPadding Padding;
    float Gap;
    float RowGap;
    float ColumnGap;
    float Margin;
    std::string AlignX;
    std::string AlignY;
    std::string SurfaceAlignX;
    std::string SurfaceAlignY;
    std::string FloatingAttachTo;
    std::string FloatingAttachX;
    std::string FloatingAttachY;
    float FloatingOffsetX;
    float FloatingOffsetY;
    float FloatingExpandWidth;
    float FloatingExpandHeight;
    int16_t FloatingZIndex;
    bool FloatingClipToParent;
    bool FloatingPointerPassthrough;
    UIColor BackgroundColor;
    UIColor TextColor;
    float Opacity;
    float CornerRadius;
    bool Visible;
    bool Enabled;
    float FontSize;
    std::string FontWeight;
    std::string TextAlign;
    float AspectRatio;
    std::string Text;
    std::string AssetCollectionId;
    std::string ImageAssetId;
    std::string OnClickHandlerId;
    std::string OnHoverHandlerId;
    std::string OnPointerEnterHandlerId;
    std::string OnPointerLeaveHandlerId;
    std::string OnPointerDownHandlerId;
    std::string OnPointerUpHandlerId;
    std::string OnDragHandlerId;
    std::string TargetEntityId;
    csp::common::Vector3 WorldOffset;
    std::string BillboardMode;
    std::string Overflow;
    std::vector<UINode> Children;

    UINode()
        : Kind(UIWidgetKind::Column)
        , Surface(UISurfaceKind::Screen)
        , Layout(UILayoutKind::Column)
        , Gap(0.0f)
        , RowGap(0.0f)
        , ColumnGap(0.0f)
        , Margin(0.0f)
        , FloatingOffsetX(0.0f)
        , FloatingOffsetY(0.0f)
        , FloatingExpandWidth(0.0f)
        , FloatingExpandHeight(0.0f)
        , FloatingZIndex(0)
        , FloatingClipToParent(false)
        , FloatingPointerPassthrough(false)
        , Opacity(1.0f)
        , CornerRadius(0.0f)
        , Visible(true)
        , Enabled(true)
        , FontSize(16.0f)
        , FontWeight("normal")
        , TextAlign("left")
        , AspectRatio(0.0f)
        , WorldOffset(0.0f, 0.0f, 0.0f)
        , Overflow("")
    {
    }
};

struct UIDrawable
{
    std::string Id;
    std::string Type;
    std::string Overflow;
    std::string ClipEscapeRootId;
    std::string Surface;
    std::string TargetEntityId;
    float SurfaceWidth;
    float SurfaceHeight;
    csp::common::Vector3 WorldOffset;
    std::string BillboardMode;
    float X;
    float Y;
    float Width;
    float Height;
    UIColor BackgroundColor;
    UIColor TextColor;
    float CornerRadius;
    float Opacity;
    std::string Text;
    float FontSize;
    std::string FontWeight;
    std::string AssetCollectionId;
    std::string ImageAssetId;
    std::string OnClickHandlerId;
    std::string OnHoverHandlerId;
    std::string OnPointerEnterHandlerId;
    std::string OnPointerLeaveHandlerId;
    std::string OnPointerDownHandlerId;
    std::string OnPointerUpHandlerId;
    std::string OnDragHandlerId;
    bool Enabled;

    UIDrawable()
        : Overflow("")
        , ClipEscapeRootId("")
        , SurfaceWidth(0.0f)
        , SurfaceHeight(0.0f)
        , WorldOffset(0.0f, 0.0f, 0.0f)
        , X(0.0f)
        , Y(0.0f)
        , Width(0.0f)
        , Height(0.0f)
        , CornerRadius(0.0f)
        , Opacity(1.0f)
        , FontSize(16.0f)
        , FontWeight("normal")
        , Enabled(true)
    {
    }
};

struct UIEntityState
{
    UINode Tree;
    std::map<std::string, UIDrawable> Drawables;
};

struct UIPendingUpdate
{
    std::string Op;
    std::string EntityId;
    UIDrawable Drawable;
};

struct UITextMeasureRequest
{
    std::string Text;
    float FontSize;
    std::string FontWeight;
};

struct ClayNodeMetadata
{
    std::string Id;
    std::string EntityId;
    std::string Surface;
    std::string Overflow;
    std::string ClipEscapeRootId;
    std::string TargetEntityId;
    float SurfaceWidth;
    float SurfaceHeight;
    csp::common::Vector3 WorldOffset;
    std::string BillboardMode;
    UIWidgetKind WidgetKind;
    UIColor BackgroundColor;
    UIColor TextColor;
    float CornerRadius;
    float Opacity;
    std::string Text;
    float FontSize;
    std::string FontWeight;
    std::string AssetCollectionId;
    std::string ImageAssetId;
    std::string OnClickHandlerId;
    std::string OnHoverHandlerId;
    std::string OnPointerEnterHandlerId;
    std::string OnPointerLeaveHandlerId;
    std::string OnPointerDownHandlerId;
    std::string OnPointerUpHandlerId;
    std::string OnDragHandlerId;
    bool Enabled;
    bool IsButtonLabel;

    ClayNodeMetadata()
        : Overflow("")
        , ClipEscapeRootId("")
        , SurfaceWidth(0.0f)
        , SurfaceHeight(0.0f)
        , WorldOffset(0.0f, 0.0f, 0.0f)
        , WidgetKind(UIWidgetKind::Column)
        , CornerRadius(0.0f)
        , Opacity(1.0f)
        , FontSize(16.0f)
        , Enabled(true)
        , IsButtonLabel(false)
    {
    }
};

struct ClayTextUserData
{
    ClayNodeMetadata* Metadata;
    std::string FontWeight;
};

Clay_Color ToClayColor(const UIColor& Color)
{
    Clay_Color Result;
    Result.r = Color.R * 255.0f;
    Result.g = Color.G * 255.0f;
    Result.b = Color.B * 255.0f;
    Result.a = Color.A * 255.0f;
    return Result;
}

Clay_CornerRadius ToClayCornerRadius(float Radius)
{
    Clay_CornerRadius Result;
    Result.topLeft = Radius;
    Result.topRight = Radius;
    Result.bottomRight = Radius;
    Result.bottomLeft = Radius;
    return Result;
}

Clay_Sizing ToClaySizing(const UISizeSpec& Width, const UISizeSpec& Height)
{
    Clay_Sizing Result;
    Result.width = Width.Mode == UISizeMode::Fixed ? CLAY_SIZING_FIXED(Width.Value)
        : (Width.Mode == UISizeMode::Grow ? CLAY_SIZING_GROW(Width.MinValue, Width.MaxValue) : CLAY_SIZING_FIT(Width.MinValue, Width.MaxValue));
    Result.height = Height.Mode == UISizeMode::Fixed ? CLAY_SIZING_FIXED(Height.Value)
        : (Height.Mode == UISizeMode::Grow ? CLAY_SIZING_GROW(Height.MinValue, Height.MaxValue) : CLAY_SIZING_FIT(Height.MinValue, Height.MaxValue));
    return Result;
}

Clay_String ToClayString(const std::string& Value)
{
    Clay_String Result;
    Result.isStaticallyAllocated = false;
    Result.length = static_cast<int32_t>(Value.size());
    Result.chars = Value.c_str();
    return Result;
}

Clay_LayoutAlignmentX ParseAlignX(const std::string& Value)
{
    if (Value == "center")
    {
        return CLAY_ALIGN_X_CENTER;
    }
    if (Value == "right")
    {
        return CLAY_ALIGN_X_RIGHT;
    }
    return CLAY_ALIGN_X_LEFT;
}

Clay_LayoutAlignmentY ParseAlignY(const std::string& Value)
{
    if (Value == "center")
    {
        return CLAY_ALIGN_Y_CENTER;
    }
    if (Value == "bottom")
    {
        return CLAY_ALIGN_Y_BOTTOM;
    }
    return CLAY_ALIGN_Y_TOP;
}

Clay_FloatingAttachPointType ParseFloatingAttachPoint(const std::string& AlignX, const std::string& AlignY)
{
    const bool bLeft = AlignX.empty() || AlignX == "left";
    const bool bCenterX = AlignX == "center";
    const bool bRight = AlignX == "right";
    const bool bTop = AlignY.empty() || AlignY == "top";
    const bool bCenterY = AlignY == "center";
    const bool bBottom = AlignY == "bottom";

    if (bLeft && bTop)
    {
        return CLAY_ATTACH_POINT_LEFT_TOP;
    }
    if (bLeft && bCenterY)
    {
        return CLAY_ATTACH_POINT_LEFT_CENTER;
    }
    if (bLeft && bBottom)
    {
        return CLAY_ATTACH_POINT_LEFT_BOTTOM;
    }
    if (bCenterX && bTop)
    {
        return CLAY_ATTACH_POINT_CENTER_TOP;
    }
    if (bCenterX && bCenterY)
    {
        return CLAY_ATTACH_POINT_CENTER_CENTER;
    }
    if (bCenterX && bBottom)
    {
        return CLAY_ATTACH_POINT_CENTER_BOTTOM;
    }
    if (bRight && bTop)
    {
        return CLAY_ATTACH_POINT_RIGHT_TOP;
    }
    if (bRight && bCenterY)
    {
        return CLAY_ATTACH_POINT_RIGHT_CENTER;
    }
    return CLAY_ATTACH_POINT_RIGHT_BOTTOM;
}

Clay_FloatingAttachToElement ParseFloatingAttachTo(const std::string& Value)
{
    if (Value == "root")
    {
        return CLAY_ATTACH_TO_ROOT;
    }
    if (Value == "parent" || Value.empty())
    {
        return CLAY_ATTACH_TO_PARENT;
    }
    return CLAY_ATTACH_TO_PARENT;
}

Clay_TextAlignment ParseTextAlignment(const std::string& Value)
{
    if (Value == "center")
    {
        return CLAY_TEXT_ALIGN_CENTER;
    }
    if (Value == "right")
    {
        return CLAY_TEXT_ALIGN_RIGHT;
    }
    return CLAY_TEXT_ALIGN_LEFT;
}

float ClampNonNegative(float Value)
{
    return Value < 0.0f ? 0.0f : Value;
}

float ApplySizeConstraints(float Value, const UISizeSpec& Size)
{
    float Result = ClampNonNegative(Value);
    if (Size.MinValue > 0.0f)
    {
        Result = std::max(Result, Size.MinValue);
    }
    if (Size.MaxValue > 0.0f)
    {
        Result = std::min(Result, Size.MaxValue);
    }
    return Result;
}

UIColor ParseColorString(const std::string& Value)
{
    UIColor Color;

    if (Value.empty() || Value[0] != '#')
    {
        return Color;
    }

    const std::string Hex = Value.substr(1);
    const bool HasAlpha = Hex.size() == 8;
    if (!(Hex.size() == 6 || HasAlpha))
    {
        return Color;
    }

    auto ParsePair = [](const std::string& Pair) -> int32_t
    {
        return static_cast<int32_t>(std::strtol(Pair.c_str(), NULL, 16));
    };

    Color.R = static_cast<float>(ParsePair(Hex.substr(0, 2))) / 255.0f;
    Color.G = static_cast<float>(ParsePair(Hex.substr(2, 2))) / 255.0f;
    Color.B = static_cast<float>(ParsePair(Hex.substr(4, 2))) / 255.0f;
    Color.A = HasAlpha ? static_cast<float>(ParsePair(Hex.substr(6, 2))) / 255.0f : 1.0f;
    return Color;
}

UIColor ApplyOpacity(const UIColor& Color, float Opacity)
{
    UIColor Result = Color;
    Result.A *= std::max(0.0f, std::min(1.0f, Opacity));
    return Result;
}

void ApplyDefaultColors(UINode& Node)
{
    switch (Node.Kind)
    {
    case UIWidgetKind::Text:
        Node.TextColor = MakeColor(1.0f, 1.0f, 1.0f, 1.0f);
        break;
    case UIWidgetKind::Button:
        Node.BackgroundColor = MakeColor(0.12f, 0.12f, 0.12f, 1.0f);
        Node.TextColor = MakeColor(1.0f, 1.0f, 1.0f, 1.0f);
        break;
    default:
        break;
    }
}

// --- QuickJS value-walking helpers ------------------------------------------

struct ScopedJSValue
{
    JSContext* Ctx;
    JSValue V;
    ScopedJSValue(JSContext* InCtx, JSValue InV)
        : Ctx(InCtx)
        , V(InV)
    {
    }
    ~ScopedJSValue()
    {
        JS_FreeValue(Ctx, V);
    }
    ScopedJSValue(const ScopedJSValue&) = delete;
    ScopedJSValue& operator=(const ScopedJSValue&) = delete;
};

bool JSIsValidObject(JSContext* Ctx, JSValueConst Val)
{
    return JS_IsObject(Val) && !JS_IsArray(Ctx, Val) && !JS_IsFunction(Ctx, Val);
}

std::string JSToStdString(JSContext* Ctx, JSValueConst Val)
{
    if (!JS_IsString(Val))
    {
        return std::string();
    }
    size_t Len = 0;
    const char* Str = JS_ToCStringLen(Ctx, &Len, Val);
    if (Str == nullptr)
    {
        return std::string();
    }
    std::string Result(Str, Len);
    JS_FreeCString(Ctx, Str);
    return Result;
}

float JSToFloatOrDefault(JSContext* Ctx, JSValueConst Val, float DefaultValue)
{
    if (!JS_IsNumber(Val))
    {
        return DefaultValue;
    }
    double Tmp = 0.0;
    if (JS_ToFloat64(Ctx, &Tmp, Val) < 0)
    {
        return DefaultValue;
    }
    return static_cast<float>(Tmp);
}

bool JSToBoolOrDefault(JSContext* Ctx, JSValueConst Val, bool DefaultValue)
{
    if (!JS_IsBool(Val))
    {
        return DefaultValue;
    }
    return JS_ToBool(Ctx, Val) > 0;
}

std::string JSGetStringOrDefault(JSContext* Ctx, JSValueConst Object, const char* Key, const std::string& DefaultValue)
{
    ScopedJSValue Val { Ctx, JS_GetPropertyStr(Ctx, Object, Key) };
    return JS_IsString(Val.V) ? JSToStdString(Ctx, Val.V) : DefaultValue;
}

float JSGetNumberOrDefault(JSContext* Ctx, JSValueConst Object, const char* Key, float DefaultValue)
{
    ScopedJSValue Val { Ctx, JS_GetPropertyStr(Ctx, Object, Key) };
    return JSToFloatOrDefault(Ctx, Val.V, DefaultValue);
}

bool JSGetBoolOrDefault(JSContext* Ctx, JSValueConst Object, const char* Key, bool DefaultValue)
{
    ScopedJSValue Val { Ctx, JS_GetPropertyStr(Ctx, Object, Key) };
    return JSToBoolOrDefault(Ctx, Val.V, DefaultValue);
}

bool JSHasOwnProperty(JSContext* Ctx, JSValueConst Object, const char* Key)
{
    // HasProperty walks the prototype chain; our JS trees are plain literals so
    // treating any defined value as "has" matches the rapidjson HasMember
    // semantics the old parser relied on.
    ScopedJSValue Val { Ctx, JS_GetPropertyStr(Ctx, Object, Key) };
    return !JS_IsUndefined(Val.V) && !JS_IsException(Val.V);
}

UISizeSpec ParseSizeSpecJS(JSContext* Ctx, JSValueConst Value, UISizeMode DefaultMode)
{
    UISizeSpec Result;
    Result.Mode = DefaultMode;

    if (JS_IsNumber(Value))
    {
        double Tmp = 0.0;
        JS_ToFloat64(Ctx, &Tmp, Value);
        Result.Mode = UISizeMode::Fixed;
        Result.Value = std::max(0.0f, static_cast<float>(Tmp));
        Result.MinValue = Result.Value;
        Result.MaxValue = Result.Value;
        return Result;
    }

    if (JS_IsString(Value))
    {
        const std::string Text = JSToStdString(Ctx, Value);
        if (Text == "grow")
        {
            Result.Mode = UISizeMode::Grow;
        }
        else
        {
            Result.Mode = UISizeMode::Fit;
        }
        return Result;
    }

    if (JSIsValidObject(Ctx, Value))
    {
        const std::string ModeText = JSGetStringOrDefault(Ctx, Value, "mode", "");
        if (ModeText == "grow")
        {
            Result.Mode = UISizeMode::Grow;
        }
        else if (ModeText == "fixed")
        {
            Result.Mode = UISizeMode::Fixed;
        }
        else if (ModeText == "fit")
        {
            Result.Mode = UISizeMode::Fit;
        }

        Result.Value = std::max(0.0f, JSGetNumberOrDefault(Ctx, Value, "value", Result.Value));
        Result.MinValue = std::max(0.0f, JSGetNumberOrDefault(Ctx, Value, "min", Result.MinValue));
        Result.MaxValue = std::max(0.0f, JSGetNumberOrDefault(Ctx, Value, "max", Result.MaxValue));

        if (Result.Mode == UISizeMode::Fixed)
        {
            if (Result.Value <= 0.0f && Result.MinValue > 0.0f)
            {
                Result.Value = Result.MinValue;
            }
            Result.MinValue = Result.Value;
            Result.MaxValue = Result.Value;
        }
        else if (Result.MaxValue > 0.0f && Result.MaxValue < Result.MinValue)
        {
            Result.MaxValue = Result.MinValue;
        }
    }

    return Result;
}

UIPadding ParsePaddingJS(JSContext* Ctx, JSValueConst Value)
{
    UIPadding Padding;
    if (JS_IsNumber(Value))
    {
        double Tmp = 0.0;
        JS_ToFloat64(Ctx, &Tmp, Value);
        const float Uniform = static_cast<float>(Tmp);
        Padding.Left = Uniform;
        Padding.Right = Uniform;
        Padding.Top = Uniform;
        Padding.Bottom = Uniform;
        return Padding;
    }

    if (JSIsValidObject(Ctx, Value))
    {
        Padding.Left = JSGetNumberOrDefault(Ctx, Value, "left", Padding.Left);
        Padding.Right = JSGetNumberOrDefault(Ctx, Value, "right", Padding.Right);
        Padding.Top = JSGetNumberOrDefault(Ctx, Value, "top", Padding.Top);
        Padding.Bottom = JSGetNumberOrDefault(Ctx, Value, "bottom", Padding.Bottom);
    }

    return Padding;
}

csp::common::Vector3 ParseVector3JS(JSContext* Ctx, JSValueConst Value)
{
    csp::common::Vector3 Result(0.0f, 0.0f, 0.0f);
    if (!JSIsValidObject(Ctx, Value))
    {
        return Result;
    }
    Result.X = JSGetNumberOrDefault(Ctx, Value, "x", Result.X);
    Result.Y = JSGetNumberOrDefault(Ctx, Value, "y", Result.Y);
    Result.Z = JSGetNumberOrDefault(Ctx, Value, "z", Result.Z);
    return Result;
}

// Recursively flatten a children array, discarding null/undefined/false and
// unwrapping nested arrays (equivalent to JSX-style children flattening). The
// caller owns the returned JSValues and must JS_FreeValue each one.
void FlattenChildrenJS(JSContext* Ctx, JSValueConst ArrayVal, std::vector<JSValue>& Out)
{
    ScopedJSValue LenVal { Ctx, JS_GetPropertyStr(Ctx, ArrayVal, "length") };
    uint32_t Length = 0;
    if (JS_ToUint32(Ctx, &Length, LenVal.V) < 0)
    {
        return;
    }

    for (uint32_t Index = 0; Index < Length; ++Index)
    {
        JSValue Elem = JS_GetPropertyUint32(Ctx, ArrayVal, Index);
        if (JS_IsArray(Ctx, Elem))
        {
            FlattenChildrenJS(Ctx, Elem, Out);
            JS_FreeValue(Ctx, Elem);
            continue;
        }
        if (JS_IsNull(Elem) || JS_IsUndefined(Elem) || (JS_IsBool(Elem) && JS_ToBool(Ctx, Elem) == 0))
        {
            JS_FreeValue(Ctx, Elem);
            continue;
        }
        Out.push_back(Elem); // transfer ownership
    }
}

std::string WidgetKindToString(UIWidgetKind Kind)
{
    switch (Kind)
    {
    case UIWidgetKind::Text:
        return "text";
    case UIWidgetKind::Image:
        return "image";
    case UIWidgetKind::Button:
        return "button";
    default:
        return "rectangle";
    }
}

std::string BuildScissorDrawableId(const std::string& BaseId, Clay_RenderCommandType CommandType)
{
    switch (CommandType)
    {
    case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
        return BaseId + ".!scissor_start";
    case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
        return BaseId + ".~scissor_end";
    default:
        return BaseId;
    }
}

const char* RenderCommandTypeToString(Clay_RenderCommandType CommandType)
{
    switch (CommandType)
    {
    case CLAY_RENDER_COMMAND_TYPE_NONE:
        return "none";
    case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
        return "rectangle";
    case CLAY_RENDER_COMMAND_TYPE_BORDER:
        return "border";
    case CLAY_RENDER_COMMAND_TYPE_TEXT:
        return "text";
    case CLAY_RENDER_COMMAND_TYPE_IMAGE:
        return "image";
    case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
        return "scissor_start";
    case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
        return "scissor_end";
    case CLAY_RENDER_COMMAND_TYPE_CUSTOM:
        return "custom";
    default:
        return "unknown";
    }
}

bool DrawablesEqual(const UIDrawable& Left, const UIDrawable& Right)
{
    return Left.Type == Right.Type
        && Left.Overflow == Right.Overflow
        && Left.ClipEscapeRootId == Right.ClipEscapeRootId
        && Left.Surface == Right.Surface
        && Left.TargetEntityId == Right.TargetEntityId
        && std::fabs(Left.SurfaceWidth - Right.SurfaceWidth) < 0.01f
        && std::fabs(Left.SurfaceHeight - Right.SurfaceHeight) < 0.01f
        && std::fabs(Left.WorldOffset.X - Right.WorldOffset.X) < 0.01f
        && std::fabs(Left.WorldOffset.Y - Right.WorldOffset.Y) < 0.01f
        && std::fabs(Left.WorldOffset.Z - Right.WorldOffset.Z) < 0.01f
        && Left.BillboardMode == Right.BillboardMode
        && std::fabs(Left.X - Right.X) < 0.01f
        && std::fabs(Left.Y - Right.Y) < 0.01f
        && std::fabs(Left.Width - Right.Width) < 0.01f
        && std::fabs(Left.Height - Right.Height) < 0.01f
        && std::fabs(Left.BackgroundColor.R - Right.BackgroundColor.R) < 0.001f
        && std::fabs(Left.BackgroundColor.G - Right.BackgroundColor.G) < 0.001f
        && std::fabs(Left.BackgroundColor.B - Right.BackgroundColor.B) < 0.001f
        && std::fabs(Left.BackgroundColor.A - Right.BackgroundColor.A) < 0.001f
        && std::fabs(Left.TextColor.R - Right.TextColor.R) < 0.001f
        && std::fabs(Left.TextColor.G - Right.TextColor.G) < 0.001f
        && std::fabs(Left.TextColor.B - Right.TextColor.B) < 0.001f
        && std::fabs(Left.TextColor.A - Right.TextColor.A) < 0.001f
        && std::fabs(Left.CornerRadius - Right.CornerRadius) < 0.01f
        && std::fabs(Left.Opacity - Right.Opacity) < 0.01f
        && Left.Text == Right.Text
        && std::fabs(Left.FontSize - Right.FontSize) < 0.01f
        && Left.FontWeight == Right.FontWeight
        && Left.AssetCollectionId == Right.AssetCollectionId
        && Left.ImageAssetId == Right.ImageAssetId
        && Left.OnClickHandlerId == Right.OnClickHandlerId
        && Left.OnHoverHandlerId == Right.OnHoverHandlerId
        && Left.OnPointerEnterHandlerId == Right.OnPointerEnterHandlerId
        && Left.OnPointerLeaveHandlerId == Right.OnPointerLeaveHandlerId
        && Left.OnPointerDownHandlerId == Right.OnPointerDownHandlerId
        && Left.OnPointerUpHandlerId == Right.OnPointerUpHandlerId
        && Left.OnDragHandlerId == Right.OnDragHandlerId
        && Left.Enabled == Right.Enabled;
}

std::string NormalizeFontWeight(const std::string& Value)
{
    std::string Result;
    Result.reserve(Value.size());
    for (char Character : Value)
    {
        Result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(Character))));
    }

    if (Result == "bold" || Result == "700" || Result == "800" || Result == "900")
    {
        return "bold";
    }

    return "normal";
}

std::string BuildTextMeasureRequestKey(const std::string& Text, float FontSize, const std::string& FontWeight)
{
    std::ostringstream Stream;
    Stream.setf(std::ios::fixed, std::ios::floatfield);
    Stream.precision(std::numeric_limits<float>::max_digits10);
    Stream << FontSize << '\n' << NormalizeFontWeight(FontWeight) << '\n' << Text;
    return Stream.str();
}

} // namespace

namespace csp::systems
{

struct NgxUIRuntime::Impl
{
    explicit Impl(csp::common::LogSystem& InLogSystem)
        : LogSystem(InLogSystem)
        , ViewportWidth(1280.0f)
        , ViewportHeight(720.0f)
        , HasWarnedFallbackMeasurement(false)
        , HasWarnedWasmCallbackLimitation(false)
        , ClayArenaMemory()
        , ClayContext(nullptr)
    {
    }

    csp::common::LogSystem& LogSystem;
    float ViewportWidth;
    float ViewportHeight;
    TextMeasureCallback MeasureCallback;
    bool HasWarnedFallbackMeasurement;
    bool HasWarnedWasmCallbackLimitation;
    bool DebugModeEnabled = false;
    std::vector<char> ClayArenaMemory;
    Clay_Context* ClayContext;
    std::map<std::string, UIEntityState> Entities;
    std::vector<UIPendingUpdate> PendingUpdates;
    std::unordered_map<std::string, csp::common::Vector2> TextMeasureCache;
    std::vector<UITextMeasureRequest> PendingTextMeasureRequests;
    std::unordered_set<std::string> PendingTextMeasureRequestKeys;

    // Handler functions captured from the JS tree at Mount time, keyed by
    // (entityId, handlerId). Dup'd JSValues owned by the runtime; freed when
    // the entity is unmounted, on re-Mount, or during Clear().
    struct EntityHandlerTable
    {
        JSContext* Ctx = nullptr;
        std::unordered_map<std::string, JSValue> Handlers;

        void Reset()
        {
            if (Ctx != nullptr)
            {
                for (std::pair<const std::string, JSValue>& Entry : Handlers)
                {
                    JS_FreeValue(Ctx, Entry.second);
                }
            }
            Handlers.clear();
        }
    };

    std::unordered_map<std::string, EntityHandlerTable> HandlerTablesByEntity;

    void ClearHandlerTable(const std::string& EntityId)
    {
        std::unordered_map<std::string, EntityHandlerTable>::iterator It = HandlerTablesByEntity.find(EntityId);
        if (It != HandlerTablesByEntity.end())
        {
            It->second.Reset();
            HandlerTablesByEntity.erase(It);
        }
    }

    void QueueTextMeasureRequest(const std::string& Text, float FontSize, const std::string& FontWeight)
    {
        const std::string Key = BuildTextMeasureRequestKey(Text, FontSize, FontWeight);
        if (PendingTextMeasureRequestKeys.find(Key) != PendingTextMeasureRequestKeys.end())
        {
            return;
        }

        PendingTextMeasureRequestKeys.insert(Key);
        PendingTextMeasureRequests.push_back(UITextMeasureRequest { Text, FontSize, NormalizeFontWeight(FontWeight) });
    }

    static void ClayErrorThunk(Clay_ErrorData ErrorData)
    {
        Impl* Self = static_cast<Impl*>(ErrorData.userData);
        if (Self == nullptr)
        {
            return;
        }

        Self->LogSystem.LogMsg(csp::common::LogLevel::Error,
            std::string("NgxUIRuntime/Clay: ")
                .append(ErrorData.errorText.chars != nullptr ? ErrorData.errorText.chars : "", static_cast<size_t>(std::max(0, ErrorData.errorText.length)))
                .c_str());
    }

    void EnsureClayContext()
    {
        if (ClayContext != nullptr)
        {
            Clay_SetCurrentContext(ClayContext);
            return;
        }

        ClayArenaMemory.resize(static_cast<size_t>(Clay_MinMemorySize()));
        Clay_ErrorHandler ErrorHandler;
        ErrorHandler.errorHandlerFunction = &Impl::ClayErrorThunk;
        ErrorHandler.userData = this;
        ClayContext = Clay_Initialize(Clay_CreateArenaWithCapacityAndMemory(ClayArenaMemory.size(), ClayArenaMemory.data()),
            Clay_Dimensions { ViewportWidth, ViewportHeight }, ErrorHandler);
        Clay_SetCurrentContext(ClayContext);
        Clay_SetMeasureTextFunction(&Impl::MeasureTextThunk, this);
        Clay_SetCullingEnabled(false);
    }

    static Clay_Dimensions MeasureTextThunk(Clay_StringSlice Text, Clay_TextElementConfig* Config, void* UserData)
    {
        Impl* Self = static_cast<Impl*>(UserData);
        Clay_Dimensions Dimensions;
        Dimensions.width = 0.0f;
        Dimensions.height = 0.0f;

        const float FontSize = Config != NULL && Config->fontSize > 0 ? static_cast<float>(Config->fontSize) : 16.0f;
        const ClayTextUserData* TextUserData = Config != NULL ? static_cast<const ClayTextUserData*>(Config->userData) : nullptr;
        const std::string FontWeight = TextUserData != nullptr ? NormalizeFontWeight(TextUserData->FontWeight) : "normal";
        const std::string RawInput
            = Text.chars != nullptr && Text.length > 0 ? std::string(Text.chars, static_cast<size_t>(Text.length)) : std::string();

        if (Self != NULL)
        {
            const std::string Key = BuildTextMeasureRequestKey(RawInput, FontSize, FontWeight);
            const std::unordered_map<std::string, csp::common::Vector2>::const_iterator Cached = Self->TextMeasureCache.find(Key);
            if (Cached != Self->TextMeasureCache.end())
            {
                Dimensions.width = Cached->second.X;
                Dimensions.height = Cached->second.Y;
                return Dimensions;
            }

#ifndef CSP_WASM
            if (Self->MeasureCallback)
            {
                const csp::common::String Input(RawInput.c_str());
                const csp::common::String Weight(FontWeight.c_str());
                const csp::common::Vector2 Size = Self->MeasureCallback(Input, FontSize, Weight);
                Self->TextMeasureCache[Key] = Size;
                Dimensions.width = Size.X;
                Dimensions.height = Size.Y;
                return Dimensions;
            }
#else
            if (Self->MeasureCallback && !Self->HasWarnedWasmCallbackLimitation)
            {
                Self->HasWarnedWasmCallbackLimitation = true;
                Self->LogSystem.LogMsg(csp::common::LogLevel::Warning,
                    "NgxUIRuntime: Direct text measurement callbacks are disabled on wasm; use the async text measurement request/result flow.");
            }
#endif

            Self->QueueTextMeasureRequest(RawInput, FontSize, FontWeight);
        }

        if (Self != NULL && !Self->HasWarnedFallbackMeasurement)
        {
            Self->HasWarnedFallbackMeasurement = true;
            Self->LogSystem.LogMsg(csp::common::LogLevel::Warning,
                "NgxUIRuntime: Text measurement unavailable; using fallback text sizing until client results are submitted.");
        }

        const float WidthMultiplier = FontWeight == "bold" ? 0.65f : 0.6f;
        Dimensions.width = static_cast<float>(Text.length) * FontSize * WidthMultiplier;
        Dimensions.height = FontSize * 1.2f;
        return Dimensions;
    }

    static constexpr int UI_TREE_MAX_DEPTH = 256;

    void RegisterUIHandler(EntityHandlerTable& Table, JSContext* Ctx, const std::string& HandlerId, JSValueConst FnValue)
    {
        Table.Ctx = Ctx;
        std::unordered_map<std::string, JSValue>::iterator It = Table.Handlers.find(HandlerId);
        if (It != Table.Handlers.end())
        {
            JS_FreeValue(Ctx, It->second);
            It->second = JS_DupValue(Ctx, FnValue);
        }
        else
        {
            Table.Handlers.emplace(HandlerId, JS_DupValue(Ctx, FnValue));
        }
    }

    void CaptureHandlerProp(JSContext* Ctx, JSValueConst Props, const char* PropName, const char* IdSuffix,
        const std::string& Path, EntityHandlerTable& Table, std::string& OutHandlerId)
    {
        ScopedJSValue FnVal { Ctx, JS_GetPropertyStr(Ctx, Props, PropName) };
        if (!JS_IsFunction(Ctx, FnVal.V))
        {
            return;
        }
        OutHandlerId = Path + ":" + IdSuffix;
        RegisterUIHandler(Table, Ctx, OutHandlerId, FnVal.V);
    }

    bool ParseNodeFromJS(JSContext* Ctx, JSValueConst NodeValue, const std::string& EntityId,
        const std::string& Path, UINode& OutNode, int Depth)
    {
        if (Depth > UI_TREE_MAX_DEPTH)
        {
            LogSystem.LogMsg(csp::common::LogLevel::Error, "NgxUIRuntime: UI tree exceeded max depth; bailing out.");
            return false;
        }

        if (!JSIsValidObject(Ctx, NodeValue))
        {
            return false;
        }

        std::string Type;
        {
            ScopedJSValue TypeVal { Ctx, JS_GetPropertyStr(Ctx, NodeValue, "type") };
            if (!JS_IsString(TypeVal.V))
            {
                return false;
            }
            Type = JSToStdString(Ctx, TypeVal.V);
        }

        if (Type == "screen")
        {
            OutNode.Kind = UIWidgetKind::ScreenRoot;
            OutNode.Surface = UISurfaceKind::Screen;
            OutNode.Layout = UILayoutKind::Column;
        }
        else if (Type == "world")
        {
            OutNode.Kind = UIWidgetKind::WorldRoot;
            OutNode.Surface = UISurfaceKind::World;
            OutNode.Layout = UILayoutKind::Column;
        }
        else if (Type == "row")
        {
            OutNode.Kind = UIWidgetKind::Row;
            OutNode.Layout = UILayoutKind::Row;
        }
        else if (Type == "column")
        {
            OutNode.Kind = UIWidgetKind::Column;
            OutNode.Layout = UILayoutKind::Column;
        }
        else if (Type == "flowRow")
        {
            OutNode.Kind = UIWidgetKind::FlowRow;
            OutNode.Layout = UILayoutKind::Row;
        }
        else if (Type == "floating")
        {
            OutNode.Kind = UIWidgetKind::Floating;
            OutNode.Layout = UILayoutKind::Column;
        }
        else if (Type == "spacer")
        {
            OutNode.Kind = UIWidgetKind::Spacer;
        }
        else if (Type == "text")
        {
            OutNode.Kind = UIWidgetKind::Text;
        }
        else if (Type == "image")
        {
            OutNode.Kind = UIWidgetKind::Image;
        }
        else if (Type == "button")
        {
            OutNode.Kind = UIWidgetKind::Button;
            OutNode.Layout = UILayoutKind::Column;
        }
        else
        {
            return false;
        }

        ScopedJSValue Props { Ctx, JS_GetPropertyStr(Ctx, NodeValue, "props") };
        const bool HasProps = JSIsValidObject(Ctx, Props.V);

        std::string Id = Path;
        if (HasProps)
        {
            ScopedJSValue KeyVal { Ctx, JS_GetPropertyStr(Ctx, Props.V, "key") };
            if (JS_IsString(KeyVal.V))
            {
                const std::string KeyStr = JSToStdString(Ctx, KeyVal.V);
                if (!KeyStr.empty())
                {
                    Id = Path + ":" + KeyStr;
                }
            }
        }
        if (Id.empty())
        {
            return false;
        }
        OutNode.Id = Id;

        ApplyDefaultColors(OutNode);

        EntityHandlerTable& Handlers = HandlerTablesByEntity[EntityId];

        if (HasProps)
        {
            if (JSHasOwnProperty(Ctx, Props.V, "width"))
            {
                ScopedJSValue V { Ctx, JS_GetPropertyStr(Ctx, Props.V, "width") };
                OutNode.Width = ParseSizeSpecJS(Ctx, V.V, UISizeMode::Fit);
            }
            if (JSHasOwnProperty(Ctx, Props.V, "height"))
            {
                ScopedJSValue V { Ctx, JS_GetPropertyStr(Ctx, Props.V, "height") };
                OutNode.Height = ParseSizeSpecJS(Ctx, V.V, UISizeMode::Fit);
            }
            if (JSHasOwnProperty(Ctx, Props.V, "padding"))
            {
                ScopedJSValue V { Ctx, JS_GetPropertyStr(Ctx, Props.V, "padding") };
                OutNode.Padding = ParsePaddingJS(Ctx, V.V);
            }
            if (JSHasOwnProperty(Ctx, Props.V, "gap"))
            {
                OutNode.Gap = JSGetNumberOrDefault(Ctx, Props.V, "gap", OutNode.Gap);
                OutNode.RowGap = OutNode.Gap;
                OutNode.ColumnGap = OutNode.Gap;
            }
            if (JSHasOwnProperty(Ctx, Props.V, "rowGap"))
            {
                OutNode.RowGap = JSGetNumberOrDefault(Ctx, Props.V, "rowGap", OutNode.RowGap);
            }
            if (JSHasOwnProperty(Ctx, Props.V, "columnGap"))
            {
                OutNode.ColumnGap = JSGetNumberOrDefault(Ctx, Props.V, "columnGap", OutNode.ColumnGap);
            }
            if (JSHasOwnProperty(Ctx, Props.V, "margin"))
            {
                OutNode.Margin = JSGetNumberOrDefault(Ctx, Props.V, "margin", OutNode.Margin);
            }
            if (JSHasOwnProperty(Ctx, Props.V, "alignX"))
            {
                const std::string AlignValue = JSGetStringOrDefault(Ctx, Props.V, "alignX", "");
                if (OutNode.Kind == UIWidgetKind::ScreenRoot || OutNode.Kind == UIWidgetKind::WorldRoot)
                {
                    OutNode.SurfaceAlignX = AlignValue;
                }
                else
                {
                    OutNode.AlignX = AlignValue;
                }
            }
            if (JSHasOwnProperty(Ctx, Props.V, "alignY"))
            {
                const std::string AlignValue = JSGetStringOrDefault(Ctx, Props.V, "alignY", "");
                if (OutNode.Kind == UIWidgetKind::ScreenRoot || OutNode.Kind == UIWidgetKind::WorldRoot)
                {
                    OutNode.SurfaceAlignY = AlignValue;
                }
                else
                {
                    OutNode.AlignY = AlignValue;
                }
            }
            if (JSHasOwnProperty(Ctx, Props.V, "backgroundColor"))
            {
                OutNode.BackgroundColor = ParseColorString(JSGetStringOrDefault(Ctx, Props.V, "backgroundColor", ""));
            }
            if (JSHasOwnProperty(Ctx, Props.V, "textColor"))
            {
                OutNode.TextColor = ParseColorString(JSGetStringOrDefault(Ctx, Props.V, "textColor", ""));
            }
            else if (JSHasOwnProperty(Ctx, Props.V, "color"))
            {
                OutNode.TextColor = ParseColorString(JSGetStringOrDefault(Ctx, Props.V, "color", ""));
            }
            else if (JSHasOwnProperty(Ctx, Props.V, "colour"))
            {
                OutNode.TextColor = ParseColorString(JSGetStringOrDefault(Ctx, Props.V, "colour", ""));
            }
            if (JSHasOwnProperty(Ctx, Props.V, "opacity"))
            {
                OutNode.Opacity = JSGetNumberOrDefault(Ctx, Props.V, "opacity", OutNode.Opacity);
            }
            if (JSHasOwnProperty(Ctx, Props.V, "cornerRadius"))
            {
                OutNode.CornerRadius = JSGetNumberOrDefault(Ctx, Props.V, "cornerRadius", OutNode.CornerRadius);
            }
            if (JSHasOwnProperty(Ctx, Props.V, "visible"))
            {
                OutNode.Visible = JSGetBoolOrDefault(Ctx, Props.V, "visible", OutNode.Visible);
            }
            if (JSHasOwnProperty(Ctx, Props.V, "enabled"))
            {
                OutNode.Enabled = JSGetBoolOrDefault(Ctx, Props.V, "enabled", OutNode.Enabled);
            }
            if (JSHasOwnProperty(Ctx, Props.V, "fontSize"))
            {
                OutNode.FontSize = JSGetNumberOrDefault(Ctx, Props.V, "fontSize", OutNode.FontSize);
            }
            if (JSHasOwnProperty(Ctx, Props.V, "fontWeight"))
            {
                ScopedJSValue FW { Ctx, JS_GetPropertyStr(Ctx, Props.V, "fontWeight") };
                if (JS_IsString(FW.V))
                {
                    OutNode.FontWeight = NormalizeFontWeight(JSToStdString(Ctx, FW.V));
                }
                else if (JS_IsNumber(FW.V))
                {
                    double FWNum = 0.0;
                    JS_ToFloat64(Ctx, &FWNum, FW.V);
                    OutNode.FontWeight = FWNum >= 600.0 ? "bold" : "normal";
                }
                else if (JS_IsBool(FW.V))
                {
                    OutNode.FontWeight = JS_ToBool(Ctx, FW.V) > 0 ? "bold" : "normal";
                }
            }
            else if (JSHasOwnProperty(Ctx, Props.V, "bold"))
            {
                OutNode.FontWeight = JSGetBoolOrDefault(Ctx, Props.V, "bold", false) ? "bold" : "normal";
            }
            if (JSHasOwnProperty(Ctx, Props.V, "aspectRatio"))
            {
                OutNode.AspectRatio = std::max(0.0f, JSGetNumberOrDefault(Ctx, Props.V, "aspectRatio", OutNode.AspectRatio));
            }
            if (JSHasOwnProperty(Ctx, Props.V, "textAlign"))
            {
                OutNode.TextAlign = JSGetStringOrDefault(Ctx, Props.V, "textAlign", OutNode.TextAlign);
            }
            else if (JSHasOwnProperty(Ctx, Props.V, "textAlignment"))
            {
                OutNode.TextAlign = JSGetStringOrDefault(Ctx, Props.V, "textAlignment", OutNode.TextAlign);
            }
            if (JSHasOwnProperty(Ctx, Props.V, "text"))
            {
                OutNode.Text = JSGetStringOrDefault(Ctx, Props.V, "text", "");
            }
            if (JSHasOwnProperty(Ctx, Props.V, "assetCollectionId"))
            {
                OutNode.AssetCollectionId = JSGetStringOrDefault(Ctx, Props.V, "assetCollectionId", "");
            }
            if (JSHasOwnProperty(Ctx, Props.V, "imageAssetId"))
            {
                OutNode.ImageAssetId = JSGetStringOrDefault(Ctx, Props.V, "imageAssetId", "");
            }

            CaptureHandlerProp(Ctx, Props.V, "onClick", "click", Path, Handlers, OutNode.OnClickHandlerId);
            CaptureHandlerProp(Ctx, Props.V, "onHover", "hover", Path, Handlers, OutNode.OnHoverHandlerId);
            CaptureHandlerProp(Ctx, Props.V, "onPointerEnter", "pointerEnter", Path, Handlers, OutNode.OnPointerEnterHandlerId);
            CaptureHandlerProp(Ctx, Props.V, "onPointerLeave", "pointerLeave", Path, Handlers, OutNode.OnPointerLeaveHandlerId);
            CaptureHandlerProp(Ctx, Props.V, "onPointerDown", "pointerDown", Path, Handlers, OutNode.OnPointerDownHandlerId);
            CaptureHandlerProp(Ctx, Props.V, "onPointerUp", "pointerUp", Path, Handlers, OutNode.OnPointerUpHandlerId);
            CaptureHandlerProp(Ctx, Props.V, "onDrag", "drag", Path, Handlers, OutNode.OnDragHandlerId);

            // targetEntity: accept either a string or an object { id: string|number }
            // (mirrors the JS normalizer's 'targetEntity'/'entity' coercion).
            if (JSHasOwnProperty(Ctx, Props.V, "targetEntity") || JSHasOwnProperty(Ctx, Props.V, "entity"))
            {
                const char* Key = JSHasOwnProperty(Ctx, Props.V, "targetEntity") ? "targetEntity" : "entity";
                ScopedJSValue EntV { Ctx, JS_GetPropertyStr(Ctx, Props.V, Key) };
                if (JS_IsString(EntV.V))
                {
                    OutNode.TargetEntityId = JSToStdString(Ctx, EntV.V);
                }
                else if (JSIsValidObject(Ctx, EntV.V))
                {
                    ScopedJSValue IdV { Ctx, JS_GetPropertyStr(Ctx, EntV.V, "id") };
                    if (JS_IsString(IdV.V))
                    {
                        OutNode.TargetEntityId = JSToStdString(Ctx, IdV.V);
                    }
                    else if (JS_IsNumber(IdV.V))
                    {
                        double Num = 0.0;
                        JS_ToFloat64(Ctx, &Num, IdV.V);
                        if (std::isfinite(Num) && Num > 0.0)
                        {
                            OutNode.TargetEntityId = std::to_string(static_cast<int64_t>(Num));
                        }
                    }
                }
            }
            if (JSHasOwnProperty(Ctx, Props.V, "targetEntityId"))
            {
                OutNode.TargetEntityId = JSGetStringOrDefault(Ctx, Props.V, "targetEntityId", OutNode.TargetEntityId);
            }
            if (JSHasOwnProperty(Ctx, Props.V, "billboardMode"))
            {
                OutNode.BillboardMode = JSGetStringOrDefault(Ctx, Props.V, "billboardMode", "");
            }
            if (JSHasOwnProperty(Ctx, Props.V, "worldOffset"))
            {
                ScopedJSValue V { Ctx, JS_GetPropertyStr(Ctx, Props.V, "worldOffset") };
                OutNode.WorldOffset = ParseVector3JS(Ctx, V.V);
            }
            if (JSHasOwnProperty(Ctx, Props.V, "attachTo"))
            {
                OutNode.FloatingAttachTo = JSGetStringOrDefault(Ctx, Props.V, "attachTo", OutNode.FloatingAttachTo);
            }
            if (JSHasOwnProperty(Ctx, Props.V, "attachX"))
            {
                OutNode.FloatingAttachX = JSGetStringOrDefault(Ctx, Props.V, "attachX", OutNode.FloatingAttachX);
            }
            if (JSHasOwnProperty(Ctx, Props.V, "attachY"))
            {
                OutNode.FloatingAttachY = JSGetStringOrDefault(Ctx, Props.V, "attachY", OutNode.FloatingAttachY);
            }
            if (JSHasOwnProperty(Ctx, Props.V, "offset"))
            {
                ScopedJSValue Offset { Ctx, JS_GetPropertyStr(Ctx, Props.V, "offset") };
                if (JSIsValidObject(Ctx, Offset.V))
                {
                    OutNode.FloatingOffsetX = JSGetNumberOrDefault(Ctx, Offset.V, "x", OutNode.FloatingOffsetX);
                    OutNode.FloatingOffsetY = JSGetNumberOrDefault(Ctx, Offset.V, "y", OutNode.FloatingOffsetY);
                }
            }
            if (JSHasOwnProperty(Ctx, Props.V, "offsetX"))
            {
                OutNode.FloatingOffsetX = JSGetNumberOrDefault(Ctx, Props.V, "offsetX", OutNode.FloatingOffsetX);
            }
            if (JSHasOwnProperty(Ctx, Props.V, "offsetY"))
            {
                OutNode.FloatingOffsetY = JSGetNumberOrDefault(Ctx, Props.V, "offsetY", OutNode.FloatingOffsetY);
            }
            if (JSHasOwnProperty(Ctx, Props.V, "expand"))
            {
                ScopedJSValue Expand { Ctx, JS_GetPropertyStr(Ctx, Props.V, "expand") };
                if (JSIsValidObject(Ctx, Expand.V))
                {
                    OutNode.FloatingExpandWidth = JSGetNumberOrDefault(Ctx, Expand.V, "width", OutNode.FloatingExpandWidth);
                    OutNode.FloatingExpandHeight = JSGetNumberOrDefault(Ctx, Expand.V, "height", OutNode.FloatingExpandHeight);
                }
            }
            if (JSHasOwnProperty(Ctx, Props.V, "expandWidth"))
            {
                OutNode.FloatingExpandWidth = JSGetNumberOrDefault(Ctx, Props.V, "expandWidth", OutNode.FloatingExpandWidth);
            }
            if (JSHasOwnProperty(Ctx, Props.V, "expandHeight"))
            {
                OutNode.FloatingExpandHeight = JSGetNumberOrDefault(Ctx, Props.V, "expandHeight", OutNode.FloatingExpandHeight);
            }
            if (JSHasOwnProperty(Ctx, Props.V, "zIndex"))
            {
                OutNode.FloatingZIndex = static_cast<int16_t>(JSGetNumberOrDefault(Ctx, Props.V, "zIndex", static_cast<float>(OutNode.FloatingZIndex)));
            }
            if (JSHasOwnProperty(Ctx, Props.V, "clipToParent"))
            {
                OutNode.FloatingClipToParent = JSGetBoolOrDefault(Ctx, Props.V, "clipToParent", OutNode.FloatingClipToParent);
            }
            if (JSHasOwnProperty(Ctx, Props.V, "pointerPassthrough"))
            {
                OutNode.FloatingPointerPassthrough = JSGetBoolOrDefault(Ctx, Props.V, "pointerPassthrough", OutNode.FloatingPointerPassthrough);
            }
            if (JSHasOwnProperty(Ctx, Props.V, "overflow"))
            {
                OutNode.Overflow = JSGetStringOrDefault(Ctx, Props.V, "overflow", "");
            }
        }

        if (OutNode.Kind == UIWidgetKind::WorldRoot && OutNode.TargetEntityId.empty())
        {
            OutNode.TargetEntityId = EntityId;
        }

        ScopedJSValue ChildrenVal { Ctx, JS_GetPropertyStr(Ctx, NodeValue, "children") };
        if (JS_IsArray(Ctx, ChildrenVal.V))
        {
            std::vector<JSValue> Flat;
            FlattenChildrenJS(Ctx, ChildrenVal.V, Flat);
            for (size_t Index = 0; Index < Flat.size(); ++Index)
            {
                UINode Child;
                Child.Surface = OutNode.Surface;
                // World placement props are declared on the world root but only travel to the client
                // via emitted drawables. The root itself often has a transparent background and is
                // dropped by Clay, so we inherit these into descendants so at least one emitted
                // drawable carries them. ParseNodeFromJS will override if the child sets its own.
                Child.TargetEntityId = OutNode.TargetEntityId;
                Child.WorldOffset = OutNode.WorldOffset;
                Child.BillboardMode = OutNode.BillboardMode;
                const std::string ChildPath = Id + "." + std::to_string(Index);
                if (ParseNodeFromJS(Ctx, Flat[Index], EntityId, ChildPath, Child, Depth + 1))
                {
                    Child.Surface = OutNode.Surface;
                    OutNode.Children.push_back(std::move(Child));
                }
            }
            for (size_t Index = 0; Index < Flat.size(); ++Index)
            {
                JS_FreeValue(Ctx, Flat[Index]);
            }
        }

        return true;
    }

    float MeasureTextWidthForNode(const std::string& Value, float FontSize, const std::string& FontWeight)
    {
        Clay_TextElementConfig Config = CLAY__DEFAULT_STRUCT;
        ClayTextUserData UserData { nullptr, NormalizeFontWeight(FontWeight) };
        Config.fontSize = static_cast<uint16_t>(std::max(1.0f, FontSize));
        Config.userData = &UserData;
        const Clay_Dimensions Dimensions
            = MeasureTextThunk(Clay_StringSlice { static_cast<int32_t>(Value.size()), Value.c_str(), Value.c_str() }, &Config, this);
        return Dimensions.width;
    }

    float ResolveNodeOuterWidth(const UINode& Node, float AvailableWidth)
    {
        if (Node.Width.Mode == UISizeMode::Fixed)
        {
            return ApplySizeConstraints(Node.Width.Value, Node.Width);
        }

        if (Node.Width.Mode == UISizeMode::Grow)
        {
            return ApplySizeConstraints(AvailableWidth, Node.Width);
        }

        switch (Node.Kind)
        {
        case UIWidgetKind::Text:
            return ApplySizeConstraints(MeasureTextWidthForNode(Node.Text, Node.FontSize, Node.FontWeight), Node.Width);
        case UIWidgetKind::Button:
            return ApplySizeConstraints(
                MeasureTextWidthForNode(Node.Text, Node.FontSize, Node.FontWeight) + Node.Padding.Left + Node.Padding.Right, Node.Width);
        case UIWidgetKind::Spacer:
            return 0.0f;
        default:
            return ApplySizeConstraints(AvailableWidth, Node.Width);
        }
    }

    float ResolveChildAvailableWidth(const UINode& Node, float ParentAvailableWidth)
    {
        const float NodeWidth = ResolveNodeOuterWidth(Node, ParentAvailableWidth);
        return ClampNonNegative(NodeWidth - Node.Padding.Left - Node.Padding.Right);
    }

    UINode ExpandFlowRows(const UINode& SourceNode, float ParentAvailableWidth)
    {
        UINode Node = SourceNode;
        const float ChildAvailableWidth = ResolveChildAvailableWidth(Node, ParentAvailableWidth);

        std::vector<UINode> ExpandedChildren;
        ExpandedChildren.reserve(Node.Children.size());
        for (size_t Index = 0; Index < Node.Children.size(); ++Index)
        {
            ExpandedChildren.push_back(ExpandFlowRows(Node.Children[Index], ChildAvailableWidth));
        }
        Node.Children = ExpandedChildren;

        if (Node.Kind != UIWidgetKind::FlowRow)
        {
            return Node;
        }

        UINode FlowContainer = Node;
        FlowContainer.Kind = UIWidgetKind::Column;
        FlowContainer.Layout = UILayoutKind::Column;
        FlowContainer.Gap = FlowContainer.RowGap;
        FlowContainer.Children.clear();

        UINode CurrentRow;
        CurrentRow.Id = Node.Id + ".__row0";
        CurrentRow.Kind = UIWidgetKind::Row;
        CurrentRow.Surface = Node.Surface;
        CurrentRow.Layout = UILayoutKind::Row;
        CurrentRow.Gap = Node.ColumnGap;
        CurrentRow.Width.Mode = UISizeMode::Grow;
        CurrentRow.Height.Mode = UISizeMode::Fit;

        float CurrentRowWidth = 0.0f;
        int32_t RowIndex = 0;

        for (size_t Index = 0; Index < Node.Children.size(); ++Index)
        {
            const UINode& Child = Node.Children[Index];
            const float ChildWidth = ResolveNodeOuterWidth(Child, ChildAvailableWidth);
            const float ProposedWidth = CurrentRow.Children.empty() ? ChildWidth : (CurrentRowWidth + Node.ColumnGap + ChildWidth);

            if (!CurrentRow.Children.empty() && ChildAvailableWidth > 0.0f && ProposedWidth > ChildAvailableWidth)
            {
                FlowContainer.Children.push_back(CurrentRow);
                ++RowIndex;
                CurrentRow = UINode();
                CurrentRow.Id = Node.Id + ".__row" + std::to_string(RowIndex);
                CurrentRow.Kind = UIWidgetKind::Row;
                CurrentRow.Surface = Node.Surface;
                CurrentRow.Layout = UILayoutKind::Row;
                CurrentRow.Gap = Node.ColumnGap;
                CurrentRow.Width.Mode = UISizeMode::Grow;
                CurrentRow.Height.Mode = UISizeMode::Fit;
                CurrentRowWidth = 0.0f;
            }

            CurrentRow.Children.push_back(Child);
            CurrentRowWidth = CurrentRow.Children.size() == 1 ? ChildWidth : (CurrentRowWidth + Node.ColumnGap + ChildWidth);
        }

        if (!CurrentRow.Children.empty())
        {
            FlowContainer.Children.push_back(CurrentRow);
        }

        return FlowContainer;
    }

    ClayNodeMetadata* CreateMetadata(std::vector<std::unique_ptr<ClayNodeMetadata>>& MetadataStorage, const std::string& EntityId, const UINode& Node,
        const std::string& DrawableId, UIWidgetKind WidgetKindOverride, const std::string& ClipEscapeRootId)
    {
        MetadataStorage.emplace_back(std::make_unique<ClayNodeMetadata>());
        ClayNodeMetadata& Meta = *MetadataStorage.back();
        Meta.Id = DrawableId;
        Meta.EntityId = EntityId;
        Meta.Surface = Node.Surface == UISurfaceKind::World ? "world" : "screen";
        Meta.Overflow = Node.Overflow;
        Meta.ClipEscapeRootId = ClipEscapeRootId;
        Meta.TargetEntityId = Node.TargetEntityId;
        Meta.WorldOffset = Node.WorldOffset;
        Meta.BillboardMode = Node.BillboardMode;
        Meta.WidgetKind = WidgetKindOverride;
        Meta.BackgroundColor = ApplyOpacity(Node.BackgroundColor, Node.Opacity);
        Meta.TextColor = ApplyOpacity(Node.TextColor, Node.Opacity);
        Meta.CornerRadius = Node.CornerRadius;
        Meta.Opacity = Node.Opacity;
        Meta.Text = Node.Text;
        Meta.FontSize = Node.FontSize;
        Meta.FontWeight = Node.FontWeight;
        Meta.AssetCollectionId = Node.AssetCollectionId;
        Meta.ImageAssetId = Node.ImageAssetId;
        Meta.OnClickHandlerId = Node.OnClickHandlerId;
        Meta.OnHoverHandlerId = Node.OnHoverHandlerId;
        Meta.OnPointerEnterHandlerId = Node.OnPointerEnterHandlerId;
        Meta.OnPointerLeaveHandlerId = Node.OnPointerLeaveHandlerId;
        Meta.OnPointerDownHandlerId = Node.OnPointerDownHandlerId;
        Meta.OnPointerUpHandlerId = Node.OnPointerUpHandlerId;
        Meta.OnDragHandlerId = Node.OnDragHandlerId;
        Meta.Enabled = Node.Enabled;
        return &Meta;
    }

    Clay_TextElementConfig* CreateTextConfig(std::vector<std::unique_ptr<Clay_TextElementConfig>>& TextConfigStorage,
        std::vector<std::unique_ptr<ClayTextUserData>>& TextUserDataStorage, const UINode& Node, ClayNodeMetadata* Metadata)
    {
        TextConfigStorage.emplace_back(std::make_unique<Clay_TextElementConfig>());
        TextUserDataStorage.emplace_back(std::make_unique<ClayTextUserData>());
        Clay_TextElementConfig& TextConfig = *TextConfigStorage.back();
        ClayTextUserData& TextUserData = *TextUserDataStorage.back();
        std::memset(&TextConfig, 0, sizeof(TextConfig));
        TextConfig.fontSize = static_cast<uint16_t>(std::max(1.0f, Node.FontSize));
        TextConfig.lineHeight = static_cast<uint16_t>(std::max(1.0f, Node.FontSize * 1.2f));
        TextConfig.textColor = ToClayColor(ApplyOpacity(Node.TextColor, Node.Opacity));
        TextConfig.textAlignment = ParseTextAlignment(Node.TextAlign);
        TextUserData.Metadata = Metadata;
        TextUserData.FontWeight = NormalizeFontWeight(Node.FontWeight);
        TextConfig.userData = &TextUserData;
        return &TextConfig;
    }

    void EmitButtonLabel(const std::string& EntityId, const UINode& Node, std::vector<std::unique_ptr<ClayNodeMetadata>>& MetadataStorage,
        std::vector<std::unique_ptr<Clay_TextElementConfig>>& TextConfigStorage,
        std::vector<std::unique_ptr<ClayTextUserData>>& TextUserDataStorage, const std::string& ClipEscapeRootId)
    {
        UINode LabelNode = Node;
        LabelNode.OnClickHandlerId.clear();
        LabelNode.OnHoverHandlerId.clear();
        LabelNode.OnPointerEnterHandlerId.clear();
        LabelNode.OnPointerLeaveHandlerId.clear();
        LabelNode.OnPointerDownHandlerId.clear();
        LabelNode.OnPointerUpHandlerId.clear();
        LabelNode.OnDragHandlerId.clear();
        LabelNode.Enabled = false;
        ClayNodeMetadata* Metadata
            = CreateMetadata(MetadataStorage, EntityId, LabelNode, Node.Id + "__label", UIWidgetKind::Text, ClipEscapeRootId);
        Metadata->IsButtonLabel = true;
        Clay_TextElementConfig* TextConfig = CreateTextConfig(TextConfigStorage, TextUserDataStorage, LabelNode, Metadata);
        Clay__OpenTextElement(ToClayString(Node.Text), TextConfig);
    }

    void EmitNodeToClay(const std::string& EntityId, const UINode& Node, std::vector<std::unique_ptr<ClayNodeMetadata>>& MetadataStorage,
        std::vector<std::unique_ptr<Clay_TextElementConfig>>& TextConfigStorage,
        std::vector<std::unique_ptr<ClayTextUserData>>& TextUserDataStorage, const std::string& InheritedClipEscapeRootId = "")
    {
        if (!Node.Visible)
        {
            return;
        }

        const std::string ClipEscapeRootId
            = !InheritedClipEscapeRootId.empty() ? InheritedClipEscapeRootId
            : (Node.Kind == UIWidgetKind::Floating && !Node.FloatingClipToParent ? Node.Id : "");

        if (Node.Kind == UIWidgetKind::Text)
        {
            ClayNodeMetadata* Metadata = CreateMetadata(MetadataStorage, EntityId, Node, Node.Id, Node.Kind, ClipEscapeRootId);
            Clay_TextElementConfig* TextConfig = CreateTextConfig(TextConfigStorage, TextUserDataStorage, Node, Metadata);
            Clay__OpenTextElement(ToClayString(Node.Text), TextConfig);
            return;
        }

        Clay_LayoutConfig LayoutConfig;
        std::memset(&LayoutConfig, 0, sizeof(LayoutConfig));
        LayoutConfig.sizing = ToClaySizing(Node.Width, Node.Height);
        LayoutConfig.padding.left = Node.Padding.Left;
        LayoutConfig.padding.right = Node.Padding.Right;
        LayoutConfig.padding.top = Node.Padding.Top;
        LayoutConfig.padding.bottom = Node.Padding.Bottom;
        LayoutConfig.childGap = Node.Gap;
        LayoutConfig.layoutDirection = Node.Layout == UILayoutKind::Row ? CLAY_LEFT_TO_RIGHT : CLAY_TOP_TO_BOTTOM;
        LayoutConfig.childAlignment.x = Node.Kind == UIWidgetKind::Button ? CLAY_ALIGN_X_CENTER : ParseAlignX(Node.AlignX);
        LayoutConfig.childAlignment.y = Node.Kind == UIWidgetKind::Button ? CLAY_ALIGN_Y_CENTER : ParseAlignY(Node.AlignY);

        Clay_ImageElementConfig ImageConfig;
        std::memset(&ImageConfig, 0, sizeof(ImageConfig));
        if (Node.Kind == UIWidgetKind::Image)
        {
            ImageConfig.imageData = const_cast<char*>(Node.ImageAssetId.c_str());
        }

        Clay_CustomElementConfig CustomConfig;
        std::memset(&CustomConfig, 0, sizeof(CustomConfig));
        const bool HasCustom = Node.Kind == UIWidgetKind::Button;
        if (HasCustom)
        {
            CustomConfig.customData = const_cast<char*>(Node.OnClickHandlerId.c_str()); // Note: only used by Clay internally, client uses hits.
        }

        Clay_FloatingElementConfig FloatingConfig = CLAY__DEFAULT_STRUCT;
        const bool HasFloating = Node.Kind == UIWidgetKind::Floating;
        if (HasFloating)
        {
            FloatingConfig.offset.x = Node.FloatingOffsetX;
            FloatingConfig.offset.y = Node.FloatingOffsetY;
            FloatingConfig.expand.width = Node.FloatingExpandWidth;
            FloatingConfig.expand.height = Node.FloatingExpandHeight;
            FloatingConfig.zIndex = Node.FloatingZIndex;
            FloatingConfig.attachTo = ParseFloatingAttachTo(Node.FloatingAttachTo);
            FloatingConfig.attachPoints.element = ParseFloatingAttachPoint(Node.FloatingAttachX, Node.FloatingAttachY);
            FloatingConfig.attachPoints.parent = ParseFloatingAttachPoint(Node.FloatingAttachX, Node.FloatingAttachY);
            FloatingConfig.clipTo = Node.FloatingClipToParent ? CLAY_CLIP_TO_ATTACHED_PARENT : CLAY_CLIP_TO_NONE;
            FloatingConfig.pointerCaptureMode
                = Node.FloatingPointerPassthrough ? CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH : CLAY_POINTER_CAPTURE_MODE_CAPTURE;
        }

        Clay_ElementDeclaration Declaration = CLAY__DEFAULT_STRUCT;
        Declaration.layout = LayoutConfig;
        Declaration.backgroundColor = ToClayColor(ApplyOpacity(Node.BackgroundColor, Node.Opacity));
        Declaration.cornerRadius = ToClayCornerRadius(Node.CornerRadius);
        if (Node.AspectRatio > 0.0f)
        {
            Declaration.aspectRatio.aspectRatio = Node.AspectRatio;
        }
        if (Node.Kind == UIWidgetKind::Image)
        {
            Declaration.image = ImageConfig;
        }
        if (HasCustom)
        {
            Declaration.custom = CustomConfig;
        }
        if (HasFloating)
        {
            Declaration.floating = FloatingConfig;
        }

        const bool bIsOverflowHidden = Node.Overflow == "hidden";
        const bool bIsScrollX = Node.Overflow == "scrollX" || Node.Overflow == "scrollXY";
        const bool bIsScrollY = Node.Overflow == "scrollY" || Node.Overflow == "scrollXY";

        if (bIsOverflowHidden || bIsScrollX || bIsScrollY)
        {
            Clay_ClipElementConfig ClipConfig;
            std::memset(&ClipConfig, 0, sizeof(ClipConfig));
            ClipConfig.horizontal = bIsOverflowHidden || bIsScrollX;
            ClipConfig.vertical = bIsOverflowHidden || bIsScrollY;
            Declaration.clip = ClipConfig;
        }

        Declaration.userData = CreateMetadata(MetadataStorage, EntityId, Node, Node.Id, Node.Kind, ClipEscapeRootId);

        Clay__OpenElementWithId(Clay_GetElementId(ToClayString(Node.Id)));
        Clay__ConfigureOpenElement(Declaration);

        if (Node.Kind == UIWidgetKind::Button)
        {
            EmitButtonLabel(EntityId, Node, MetadataStorage, TextConfigStorage, TextUserDataStorage, ClipEscapeRootId);
        }
        else
        {
            for (size_t Index = 0; Index < Node.Children.size(); ++Index)
            {
                EmitNodeToClay(EntityId, Node.Children[Index], MetadataStorage, TextConfigStorage, TextUserDataStorage, ClipEscapeRootId);
            }
        }

        Clay__CloseElement();
    }

    std::map<std::string, UIDrawable> BuildDrawables(const std::string& EntityId, const UINode& Root)
    {
        std::map<std::string, UIDrawable> Drawables;
        std::vector<std::unique_ptr<ClayNodeMetadata>> MetadataStorage;
        std::vector<std::unique_ptr<Clay_TextElementConfig>> TextConfigStorage;
        std::vector<std::unique_ptr<ClayTextUserData>> TextUserDataStorage;
        EnsureClayContext();

        const float SurfaceWidth = Root.Surface == UISurfaceKind::Screen
            ? ViewportWidth
            : (Root.Width.Mode == UISizeMode::Fixed && Root.Width.Value > 0.0f ? Root.Width.Value : 1024.0f);
        const float SurfaceHeight = Root.Surface == UISurfaceKind::Screen
            ? ViewportHeight
            : (Root.Height.Mode == UISizeMode::Fixed && Root.Height.Value > 0.0f ? Root.Height.Value : 768.0f);

        Clay_SetLayoutDimensions(Clay_Dimensions { SurfaceWidth, SurfaceHeight });
        Clay_BeginLayout();

        UINode Host = Root;
        Host.Id = Root.Id + "__surface";
        Host.Kind = Root.Surface == UISurfaceKind::Screen ? UIWidgetKind::ScreenRoot : UIWidgetKind::WorldRoot;
        Host.Layout = UILayoutKind::Column;
        Host.Width.Mode = UISizeMode::Fixed;
        Host.Width.Value = SurfaceWidth;
        Host.Height.Mode = UISizeMode::Fixed;
        Host.Height.Value = SurfaceHeight;
        Host.Padding.Left = Root.Margin;
        Host.Padding.Right = Root.Margin;
        Host.Padding.Top = Root.Margin;
        Host.Padding.Bottom = Root.Margin;
        Host.Gap = 0.0f;
        Host.AlignX = Root.SurfaceAlignX;
        Host.AlignY = Root.SurfaceAlignY;
        Host.BackgroundColor = UIColor();
        Host.Children.clear();
        Host.Children.push_back(Root);

        EmitNodeToClay(EntityId, Host, MetadataStorage, TextConfigStorage, TextUserDataStorage);

        const Clay_RenderCommandArray RenderCommands = Clay_EndLayout();
        std::map<std::string, int32_t> TotalTextCommandCountById;
        for (int32_t Index = 0; Index < RenderCommands.length; ++Index)
        {
            Clay_RenderCommand* Command = Clay_RenderCommandArray_Get(const_cast<Clay_RenderCommandArray*>(&RenderCommands), Index);
            if (Command == nullptr || Command->userData == nullptr || Command->commandType != CLAY_RENDER_COMMAND_TYPE_TEXT)
            {
                continue;
            }

            const ClayTextUserData* TextUserData = static_cast<const ClayTextUserData*>(Command->userData);
            const ClayNodeMetadata& Meta = *TextUserData->Metadata;
            ++TotalTextCommandCountById[Meta.Id];
        }

        std::map<std::string, int32_t> SeenTextCommandCountById;
        for (int32_t Index = 0; Index < RenderCommands.length; ++Index)
        {
            Clay_RenderCommand* Command = Clay_RenderCommandArray_Get(const_cast<Clay_RenderCommandArray*>(&RenderCommands), Index);
            if (Command == nullptr || Command->userData == nullptr)
            {
                continue;
            }

            const ClayNodeMetadata* MetaPtr = nullptr;
            if (Command->commandType == CLAY_RENDER_COMMAND_TYPE_TEXT)
            {
                const ClayTextUserData* TextUserData = static_cast<const ClayTextUserData*>(Command->userData);
                MetaPtr = TextUserData != nullptr ? TextUserData->Metadata : nullptr;
            }
            else
            {
                MetaPtr = static_cast<const ClayNodeMetadata*>(Command->userData);
            }

            if (MetaPtr == nullptr)
            {
                continue;
            }

            const ClayNodeMetadata& Meta = *MetaPtr;
            if (Meta.Id == Host.Id)
            {
                continue;
            }

            if (Meta.IsButtonLabel && Command->commandType == CLAY_RENDER_COMMAND_TYPE_TEXT)
            {
                continue;
            }

            UIDrawable Drawable;
            Drawable.Id = Meta.Id;
            Drawable.Type = WidgetKindToString(Meta.WidgetKind);
            Drawable.Overflow = Meta.Overflow;
            Drawable.ClipEscapeRootId = Meta.ClipEscapeRootId;
            Drawable.Surface = Meta.Surface;
            Drawable.TargetEntityId = Meta.TargetEntityId;
            Drawable.SurfaceWidth = SurfaceWidth;
            Drawable.SurfaceHeight = SurfaceHeight;
            Drawable.WorldOffset = Meta.WorldOffset;
            Drawable.BillboardMode = Meta.BillboardMode;
            Drawable.X = Command->boundingBox.x;
            Drawable.Y = Command->boundingBox.y;
            Drawable.Width = Command->boundingBox.width;
            Drawable.Height = Command->boundingBox.height;
            Drawable.BackgroundColor = Meta.BackgroundColor;
            Drawable.TextColor = Meta.TextColor;
            Drawable.CornerRadius = Meta.CornerRadius;
            Drawable.Opacity = Meta.Opacity;
            Drawable.Text = Meta.Text;
            Drawable.FontSize = Meta.FontSize;
            Drawable.FontWeight = Meta.FontWeight;
            Drawable.AssetCollectionId = Meta.AssetCollectionId;
            Drawable.ImageAssetId = Meta.ImageAssetId;
            Drawable.OnClickHandlerId = Meta.OnClickHandlerId;
            Drawable.OnHoverHandlerId = Meta.OnHoverHandlerId;
            Drawable.OnPointerEnterHandlerId = Meta.OnPointerEnterHandlerId;
            Drawable.OnPointerLeaveHandlerId = Meta.OnPointerLeaveHandlerId;
            Drawable.OnPointerDownHandlerId = Meta.OnPointerDownHandlerId;
            Drawable.OnPointerUpHandlerId = Meta.OnPointerUpHandlerId;
            Drawable.OnDragHandlerId = Meta.OnDragHandlerId;
            Drawable.Enabled = Meta.Enabled;

            if (Command->commandType == CLAY_RENDER_COMMAND_TYPE_CUSTOM)
            {
                Drawable.Type = Meta.WidgetKind == UIWidgetKind::Button ? "button" : WidgetKindToString(Meta.WidgetKind);
            }
            else if (Command->commandType == CLAY_RENDER_COMMAND_TYPE_SCISSOR_START)
            {
                Drawable.Id = BuildScissorDrawableId(Meta.Id, Command->commandType);
                Drawable.Type = "scissor_start";
                Drawable.Text.clear();
                Drawable.OnClickHandlerId.clear();
                Drawable.OnHoverHandlerId.clear();
                Drawable.OnPointerEnterHandlerId.clear();
                Drawable.OnPointerLeaveHandlerId.clear();
                Drawable.OnPointerDownHandlerId.clear();
                Drawable.OnPointerUpHandlerId.clear();
                Drawable.OnDragHandlerId.clear();
                Drawable.Enabled = false;
            }
            else if (Command->commandType == CLAY_RENDER_COMMAND_TYPE_SCISSOR_END)
            {
                Drawable.Id = BuildScissorDrawableId(Meta.Id, Command->commandType);
                Drawable.Type = "scissor_end";
                Drawable.Text.clear();
                Drawable.OnClickHandlerId.clear();
                Drawable.OnHoverHandlerId.clear();
                Drawable.OnPointerEnterHandlerId.clear();
                Drawable.OnPointerLeaveHandlerId.clear();
                Drawable.OnPointerDownHandlerId.clear();
                Drawable.OnPointerUpHandlerId.clear();
                Drawable.OnDragHandlerId.clear();
                Drawable.Enabled = false;
            }
            else if (Command->commandType == CLAY_RENDER_COMMAND_TYPE_TEXT)
            {
                Drawable.Type = "text";
                const int32_t TotalTextCommands = TotalTextCommandCountById[Meta.Id];
                const int32_t SeenTextCommands = SeenTextCommandCountById[Meta.Id]++;
                if (TotalTextCommands > 1)
                {
                    Drawable.Id = Meta.Id + ".__line" + std::to_string(SeenTextCommands);
                }

                const Clay_StringSlice& StringContents = Command->renderData.text.stringContents;
                if (StringContents.chars != nullptr && StringContents.length >= 0)
                {
                    Drawable.Text.assign(StringContents.chars, static_cast<size_t>(StringContents.length));
                }
                else
                {
                    Drawable.Text.clear();
                }
                Drawable.FontSize = static_cast<float>(Command->renderData.text.fontSize);
                Drawable.FontWeight = Meta.FontWeight;
            }
            else if (Command->commandType == CLAY_RENDER_COMMAND_TYPE_IMAGE)
            {
                Drawable.Type = "image";
            }
            else if (Meta.WidgetKind == UIWidgetKind::Button)
            {
                Drawable.Type = "button";
            }
            else
            {
                Drawable.Type = "rectangle";
            }

            const std::map<std::string, UIDrawable>::iterator Existing = Drawables.find(Drawable.Id);
            if (Existing != Drawables.end())
            {
                std::ostringstream Message;
                Message << "NgxUIRuntime: Duplicate drawable id '" << Drawable.Id << "' while building entity '" << EntityId
                        << "'. Existing type=" << Existing->second.Type << " new type=" << Drawable.Type
                        << " existing text='" << Existing->second.Text << "' new text='" << Drawable.Text
                        << "' commandType=" << RenderCommandTypeToString(Command->commandType)
                        << " metaWidgetKind=" << WidgetKindToString(Meta.WidgetKind)
                        << " x=" << Drawable.X << " y=" << Drawable.Y
                        << " width=" << Drawable.Width << " height=" << Drawable.Height;
                LogSystem.LogMsg(csp::common::LogLevel::Warning, Message.str().c_str());
            }

            Drawables[Drawable.Id] = Drawable;
        }

        return Drawables;
    }

    void QueueDiff(const std::string& EntityId, const std::map<std::string, UIDrawable>& Previous, const std::map<std::string, UIDrawable>& Current)
    {
        for (std::map<std::string, UIDrawable>::const_iterator It = Previous.begin(); It != Previous.end(); ++It)
        {
            if (Current.find(It->first) == Current.end())
            {
                UIPendingUpdate Update;
                Update.Op = "remove";
                Update.EntityId = EntityId;
                Update.Drawable = It->second;
                PendingUpdates.push_back(Update);
            }
        }

        for (std::map<std::string, UIDrawable>::const_iterator It = Current.begin(); It != Current.end(); ++It)
        {
            const std::map<std::string, UIDrawable>::const_iterator PreviousIt = Previous.find(It->first);
            if (PreviousIt == Previous.end())
            {
                UIPendingUpdate Update;
                Update.Op = "add";
                Update.EntityId = EntityId;
                Update.Drawable = It->second;
                PendingUpdates.push_back(Update);
            }
            else if (!DrawablesEqual(PreviousIt->second, It->second))
            {
                UIPendingUpdate Update;
                Update.Op = "update";
                Update.EntityId = EntityId;
                Update.Drawable = It->second;
                PendingUpdates.push_back(Update);
            }
        }
    }

    // Re-layout a stored UINode (used by viewport changes and text-measure
    // callbacks — avoids re-walking JS on every pixel change).
    bool RelayoutStoredTree(const std::string& EntityId)
    {
        const std::map<std::string, UIEntityState>::iterator It = Entities.find(EntityId);
        if (It == Entities.end())
        {
            return false;
        }

        const float RootAvailableWidth = It->second.Tree.Surface == UISurfaceKind::Screen
            ? ViewportWidth
            : (It->second.Tree.Width.Mode == UISizeMode::Fixed && It->second.Tree.Width.Value > 0.0f ? It->second.Tree.Width.Value : 1024.0f);
        UINode Expanded = ExpandFlowRows(It->second.Tree, RootAvailableWidth);
        const std::map<std::string, UIDrawable> NextDrawables = BuildDrawables(EntityId, Expanded);
        QueueDiff(EntityId, It->second.Drawables, NextDrawables);
        It->second.Drawables = NextDrawables;
        return true;
    }

    void WriteColor(rapidjson::Value& Target, const UIColor& Color, rapidjson::Document::AllocatorType& Allocator) const
    {
        Target.SetObject();
        Target.AddMember("r", Color.R, Allocator);
        Target.AddMember("g", Color.G, Allocator);
        Target.AddMember("b", Color.B, Allocator);
        Target.AddMember("a", Color.A, Allocator);
    }

    rapidjson::Value DrawableToJsonValue(const UIDrawable& Drawable, rapidjson::Document::AllocatorType& Allocator) const
    {
        rapidjson::Value Value(rapidjson::kObjectType);
        Value.AddMember("id", rapidjson::Value(Drawable.Id.c_str(), Allocator), Allocator);
        Value.AddMember("type", rapidjson::Value(Drawable.Type.c_str(), Allocator), Allocator);
        Value.AddMember("overflow", rapidjson::Value(Drawable.Overflow.c_str(), Allocator), Allocator);
        Value.AddMember("clipEscapeRootId", rapidjson::Value(Drawable.ClipEscapeRootId.c_str(), Allocator), Allocator);
        Value.AddMember("surface", rapidjson::Value(Drawable.Surface.c_str(), Allocator), Allocator);
        Value.AddMember("targetEntityId", rapidjson::Value(Drawable.TargetEntityId.c_str(), Allocator), Allocator);
        Value.AddMember("surfaceWidth", Drawable.SurfaceWidth, Allocator);
        Value.AddMember("surfaceHeight", Drawable.SurfaceHeight, Allocator);
        rapidjson::Value WorldOffset(rapidjson::kObjectType);
        WorldOffset.AddMember("x", Drawable.WorldOffset.X, Allocator);
        WorldOffset.AddMember("y", Drawable.WorldOffset.Y, Allocator);
        WorldOffset.AddMember("z", Drawable.WorldOffset.Z, Allocator);
        Value.AddMember("worldOffset", WorldOffset, Allocator);
        Value.AddMember("billboardMode", rapidjson::Value(Drawable.BillboardMode.c_str(), Allocator), Allocator);
        Value.AddMember("x", Drawable.X, Allocator);
        Value.AddMember("y", Drawable.Y, Allocator);
        Value.AddMember("width", Drawable.Width, Allocator);
        Value.AddMember("height", Drawable.Height, Allocator);
        rapidjson::Value BackgroundColor(rapidjson::kObjectType);
        WriteColor(BackgroundColor, Drawable.BackgroundColor, Allocator);
        Value.AddMember("backgroundColor", BackgroundColor, Allocator);
        rapidjson::Value TextColor(rapidjson::kObjectType);
        WriteColor(TextColor, Drawable.TextColor, Allocator);
        Value.AddMember("textColor", TextColor, Allocator);
        Value.AddMember("cornerRadius", Drawable.CornerRadius, Allocator);
        Value.AddMember("opacity", Drawable.Opacity, Allocator);
        Value.AddMember("text", rapidjson::Value(Drawable.Text.c_str(), Allocator), Allocator);
        Value.AddMember("fontSize", Drawable.FontSize, Allocator);
        Value.AddMember("fontWeight", rapidjson::Value(Drawable.FontWeight.c_str(), Allocator), Allocator);
        Value.AddMember("assetCollectionId", rapidjson::Value(Drawable.AssetCollectionId.c_str(), Allocator), Allocator);
        Value.AddMember("imageAssetId", rapidjson::Value(Drawable.ImageAssetId.c_str(), Allocator), Allocator);
        Value.AddMember("handlerId", rapidjson::Value(Drawable.OnClickHandlerId.c_str(), Allocator), Allocator);
        Value.AddMember("onClickHandlerId", rapidjson::Value(Drawable.OnClickHandlerId.c_str(), Allocator), Allocator);
        Value.AddMember("onHoverHandlerId", rapidjson::Value(Drawable.OnHoverHandlerId.c_str(), Allocator), Allocator);
        Value.AddMember("onPointerEnterHandlerId", rapidjson::Value(Drawable.OnPointerEnterHandlerId.c_str(), Allocator), Allocator);
        Value.AddMember("onPointerLeaveHandlerId", rapidjson::Value(Drawable.OnPointerLeaveHandlerId.c_str(), Allocator), Allocator);
        Value.AddMember("onPointerDownHandlerId", rapidjson::Value(Drawable.OnPointerDownHandlerId.c_str(), Allocator), Allocator);
        Value.AddMember("onPointerUpHandlerId", rapidjson::Value(Drawable.OnPointerUpHandlerId.c_str(), Allocator), Allocator);
        Value.AddMember("onDragHandlerId", rapidjson::Value(Drawable.OnDragHandlerId.c_str(), Allocator), Allocator);
        Value.AddMember("enabled", Drawable.Enabled, Allocator);
        return Value;
    }
};

NgxUIRuntime::NgxUIRuntime(csp::common::LogSystem& InLogSystem)
    : Pimpl(new Impl(InLogSystem))
{
}

NgxUIRuntime::~NgxUIRuntime() = default;

void NgxUIRuntime::Clear()
{
    for (std::map<std::string, UIEntityState>::const_iterator It = Pimpl->Entities.begin(); It != Pimpl->Entities.end(); ++It)
    {
        Pimpl->QueueDiff(It->first, It->second.Drawables, std::map<std::string, UIDrawable>());
    }
    Pimpl->Entities.clear();
    for (std::unordered_map<std::string, Impl::EntityHandlerTable>::iterator It = Pimpl->HandlerTablesByEntity.begin();
         It != Pimpl->HandlerTablesByEntity.end(); ++It)
    {
        It->second.Reset();
    }
    Pimpl->HandlerTablesByEntity.clear();
    Pimpl->PendingTextMeasureRequests.clear();
    Pimpl->PendingTextMeasureRequestKeys.clear();
}

void NgxUIRuntime::SetViewportSize(float Width, float Height)
{
    if (!std::isfinite(Width) || !std::isfinite(Height) || Width <= 0.0f || Height <= 0.0f)
    {
        return;
    }

    Pimpl->ViewportWidth = Width;
    Pimpl->ViewportHeight = Height;

    std::vector<std::string> EntityIds;
    EntityIds.reserve(Pimpl->Entities.size());
    for (std::map<std::string, UIEntityState>::const_iterator It = Pimpl->Entities.begin(); It != Pimpl->Entities.end(); ++It)
    {
        EntityIds.push_back(It->first);
    }
    for (size_t Index = 0; Index < EntityIds.size(); ++Index)
    {
        Pimpl->RelayoutStoredTree(EntityIds[Index]);
    }
}

void NgxUIRuntime::SetTextMeasureCallback(TextMeasureCallback InCallback)
{
    Pimpl->MeasureCallback = InCallback;
    Pimpl->HasWarnedFallbackMeasurement = false;
    Pimpl->HasWarnedWasmCallbackLimitation = false;
    Pimpl->PendingTextMeasureRequests.clear();
    Pimpl->PendingTextMeasureRequestKeys.clear();
    Pimpl->TextMeasureCache.clear();
}

std::string NgxUIRuntime::DrainPendingTextMeasureRequestsJson()
{
    rapidjson::Document Document;
    Document.SetArray();
    rapidjson::Document::AllocatorType& Allocator = Document.GetAllocator();

    for (size_t Index = 0; Index < Pimpl->PendingTextMeasureRequests.size(); ++Index)
    {
        const UITextMeasureRequest& Request = Pimpl->PendingTextMeasureRequests[Index];
        rapidjson::Value Entry(rapidjson::kObjectType);
        Entry.AddMember("text", rapidjson::Value(Request.Text.c_str(), Allocator), Allocator);
        Entry.AddMember("fontSize", Request.FontSize, Allocator);
        Entry.AddMember("fontWeight", rapidjson::Value(Request.FontWeight.c_str(), Allocator), Allocator);
        Document.PushBack(Entry, Allocator);
    }

    Pimpl->PendingTextMeasureRequests.clear();
    Pimpl->PendingTextMeasureRequestKeys.clear();

    rapidjson::StringBuffer Buffer;
    rapidjson::Writer<rapidjson::StringBuffer> Writer(Buffer);
    Document.Accept(Writer);
    return Buffer.GetString();
}

bool NgxUIRuntime::SubmitTextMeasureResultsJson(const std::string& ResultsJson)
{
    rapidjson::Document Document;
    Document.Parse(ResultsJson.c_str());
    if (Document.HasParseError() || !Document.IsArray())
    {
        return false;
    }

    bool bAppliedAnyResults = false;
    for (rapidjson::SizeType Index = 0; Index < Document.Size(); ++Index)
    {
        const rapidjson::Value& Entry = Document[Index];
        if (!Entry.IsObject() || !Entry.HasMember("text") || !Entry["text"].IsString() || !Entry.HasMember("fontSize")
            || !Entry["fontSize"].IsNumber() || !Entry.HasMember("width") || !Entry["width"].IsNumber() || !Entry.HasMember("height")
            || !Entry["height"].IsNumber())
        {
            continue;
        }

        const std::string Text = Entry["text"].GetString();
        const float FontSize = static_cast<float>(Entry["fontSize"].GetDouble());
        const std::string FontWeight = Entry.HasMember("fontWeight") && Entry["fontWeight"].IsString()
            ? NormalizeFontWeight(Entry["fontWeight"].GetString())
            : std::string("normal");
        const float Width = std::max(0.0f, static_cast<float>(Entry["width"].GetDouble()));
        const float Height = std::max(0.0f, static_cast<float>(Entry["height"].GetDouble()));
        Pimpl->TextMeasureCache[BuildTextMeasureRequestKey(Text, FontSize, FontWeight)] = csp::common::Vector2(Width, Height);
        bAppliedAnyResults = true;
    }

    if (bAppliedAnyResults)
    {
        Pimpl->HasWarnedFallbackMeasurement = false;
        std::vector<std::string> EntityIds;
        EntityIds.reserve(Pimpl->Entities.size());
        for (std::map<std::string, UIEntityState>::const_iterator It = Pimpl->Entities.begin(); It != Pimpl->Entities.end(); ++It)
        {
            EntityIds.push_back(It->first);
        }

        for (size_t Index = 0; Index < EntityIds.size(); ++Index)
        {
            Pimpl->RelayoutStoredTree(EntityIds[Index]);
        }
    }

    return true;
}

bool NgxUIRuntime::Mount(const std::string& EntityId, JSContext* Ctx, JSValueConst TreeValue)
{
    if (Ctx == nullptr)
    {
        return false;
    }

    // Drop any handler references from a previous mount of this entity before
    // the walker captures new ones. The old JSValues are released here.
    Pimpl->ClearHandlerTable(EntityId);

    UINode Root;
    if (!Pimpl->ParseNodeFromJS(Ctx, TreeValue, EntityId, std::string("root"), Root, 0))
    {
        return false;
    }

    const float RootAvailableWidth = Root.Surface == UISurfaceKind::Screen
        ? Pimpl->ViewportWidth
        : (Root.Width.Mode == UISizeMode::Fixed && Root.Width.Value > 0.0f ? Root.Width.Value : 1024.0f);
    UINode Expanded = Pimpl->ExpandFlowRows(Root, RootAvailableWidth);

    const std::map<std::string, UIDrawable> NextDrawables = Pimpl->BuildDrawables(EntityId, Expanded);
    UIEntityState& State = Pimpl->Entities[EntityId];
    Pimpl->QueueDiff(EntityId, State.Drawables, NextDrawables);
    State.Tree = Root;
    State.Drawables = NextDrawables;
    return true;
}

bool NgxUIRuntime::Unmount(const std::string& EntityId)
{
    const std::map<std::string, UIEntityState>::iterator It = Pimpl->Entities.find(EntityId);
    if (It == Pimpl->Entities.end())
    {
        Pimpl->ClearHandlerTable(EntityId);
        return false;
    }

    Pimpl->QueueDiff(EntityId, It->second.Drawables, std::map<std::string, UIDrawable>());
    Pimpl->Entities.erase(It);
    Pimpl->ClearHandlerTable(EntityId);
    return true;
}

JSValueConst NgxUIRuntime::GetHandler(const std::string& EntityId, const std::string& HandlerId) const
{
    const std::unordered_map<std::string, Impl::EntityHandlerTable>::const_iterator It = Pimpl->HandlerTablesByEntity.find(EntityId);
    if (It == Pimpl->HandlerTablesByEntity.end())
    {
        return JS_UNDEFINED;
    }
    const std::unordered_map<std::string, JSValue>::const_iterator FnIt = It->second.Handlers.find(HandlerId);
    if (FnIt == It->second.Handlers.end())
    {
        return JS_UNDEFINED;
    }
    return FnIt->second;
}

std::string NgxUIRuntime::DrainPendingUpdatesJson()
{
    rapidjson::Document Document;
    Document.SetArray();
    rapidjson::Document::AllocatorType& Allocator = Document.GetAllocator();

    for (size_t Index = 0; Index < Pimpl->PendingUpdates.size(); ++Index)
    {
        const UIPendingUpdate& Update = Pimpl->PendingUpdates[Index];
        rapidjson::Value Entry(rapidjson::kObjectType);
        Entry.AddMember("op", rapidjson::Value(Update.Op.c_str(), Allocator), Allocator);
        Entry.AddMember("entityId", rapidjson::Value(Update.EntityId.c_str(), Allocator), Allocator);
        Entry.AddMember("drawable", Pimpl->DrawableToJsonValue(Update.Drawable, Allocator), Allocator);
        Document.PushBack(Entry, Allocator);
    }

    Pimpl->PendingUpdates.clear();
    rapidjson::StringBuffer Buffer;
    rapidjson::Writer<rapidjson::StringBuffer> Writer(Buffer);
    Document.Accept(Writer);
    return Buffer.GetString();
}

void NgxUIRuntime::SetDebugModeEnabled(bool bEnabled)
{
    Pimpl->DebugModeEnabled = bEnabled;
    Clay_SetDebugModeEnabled(bEnabled);
}

bool NgxUIRuntime::IsDebugModeEnabled() const
{
    return Pimpl->DebugModeEnabled;
}

size_t NgxUIRuntime::UnmountEntitiesNotIn(const std::unordered_set<std::string>& ActiveEntityIds)
{
    std::vector<std::string> StaleEntityIds;
    for (const auto& Pair : Pimpl->Entities)
    {
        if (ActiveEntityIds.find(Pair.first) == ActiveEntityIds.end())
        {
            StaleEntityIds.push_back(Pair.first);
        }
    }

    for (const std::string& EntityId : StaleEntityIds)
    {
        const std::map<std::string, UIEntityState>::iterator It = Pimpl->Entities.find(EntityId);
        if (It == Pimpl->Entities.end())
        {
            continue;
        }
        Pimpl->QueueDiff(EntityId, It->second.Drawables, std::map<std::string, UIDrawable>());
        Pimpl->Entities.erase(It);
        Pimpl->ClearHandlerTable(EntityId);
    }

    return StaleEntityIds.size();
}

std::string NgxUIRuntime::GetDrawablesJson(const std::string& EntityId) const
{
    rapidjson::Document Document;
    Document.SetArray();
    rapidjson::Document::AllocatorType& Allocator = Document.GetAllocator();

    const std::map<std::string, UIEntityState>::const_iterator It = Pimpl->Entities.find(EntityId);
    if (It == Pimpl->Entities.end())
    {
        rapidjson::StringBuffer Buffer;
        rapidjson::Writer<rapidjson::StringBuffer> Writer(Buffer);
        Document.Accept(Writer);
        return Buffer.GetString();
    }

    for (std::map<std::string, UIDrawable>::const_iterator DrawableIt = It->second.Drawables.begin(); DrawableIt != It->second.Drawables.end();
         ++DrawableIt)
    {
        Document.PushBack(Pimpl->DrawableToJsonValue(DrawableIt->second, Allocator), Allocator);
    }

    rapidjson::StringBuffer Buffer;
    rapidjson::Writer<rapidjson::StringBuffer> Writer(Buffer);
    Document.Accept(Writer);
    return Buffer.GetString();
}

} // namespace csp::systems
