#include "Multiplayer/NgxScript/NgxUIRuntime.h"

#include "CSP/Common/Systems/Log/LogSystem.h"

#define CLAY_IMPLEMENTATION
#include "clay.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <cmath>
#include <deque>
#include <map>
#include <memory>
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
    TextHost,
    Text,
    Image,
    Button,
};

struct UISizeSpec
{
    UISizeMode Mode;
    float Value;

    UISizeSpec()
        : Mode(UISizeMode::Fit)
        , Value(0.0f)
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
    std::string Text;
    std::string AssetCollectionId;
    std::string ImageAssetId;
    std::string HandlerId;
    std::string TargetEntityId;
    csp::common::Vector3 WorldOffset;
    std::string BillboardMode;
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
        , WorldOffset(0.0f, 0.0f, 0.0f)
    {
    }
};

struct UIDrawable
{
    std::string Id;
    std::string Type;
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
    std::string AssetCollectionId;
    std::string ImageAssetId;
    std::string HandlerId;
    bool Enabled;

    UIDrawable()
        : SurfaceWidth(0.0f)
        , SurfaceHeight(0.0f)
        , WorldOffset(0.0f, 0.0f, 0.0f)
        , X(0.0f)
        , Y(0.0f)
        , Width(0.0f)
        , Height(0.0f)
        , CornerRadius(0.0f)
        , Opacity(1.0f)
        , FontSize(16.0f)
        , Enabled(true)
    {
    }
};

struct UIEntityState
{
    std::string TreeJson;
    std::map<std::string, UIDrawable> Drawables;
};

struct UIPendingUpdate
{
    std::string Op;
    std::string EntityId;
    UIDrawable Drawable;
};

struct ClayNodeMetadata
{
    std::string Id;
    std::string EntityId;
    std::string Surface;
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
    std::string AssetCollectionId;
    std::string ImageAssetId;
    std::string HandlerId;
    bool Enabled;

    ClayNodeMetadata()
        : SurfaceWidth(0.0f)
        , SurfaceHeight(0.0f)
        , WorldOffset(0.0f, 0.0f, 0.0f)
        , WidgetKind(UIWidgetKind::Column)
        , CornerRadius(0.0f)
        , Opacity(1.0f)
        , FontSize(16.0f)
        , Enabled(true)
    {
    }
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
                                                   : (Width.Mode == UISizeMode::Grow ? CLAY_SIZING_GROW() : CLAY_SIZING_FIT());
    Result.height = Height.Mode == UISizeMode::Fixed ? CLAY_SIZING_FIXED(Height.Value)
                                                     : (Height.Mode == UISizeMode::Grow ? CLAY_SIZING_GROW() : CLAY_SIZING_FIT());
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

float ClampNonNegative(float Value) { return Value < 0.0f ? 0.0f : Value; }

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

    auto ParsePair = [](const std::string& Pair) -> int32_t { return static_cast<int32_t>(std::strtol(Pair.c_str(), NULL, 16)); };

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

UISizeSpec ParseSizeSpec(const rapidjson::Value& Value, UISizeMode DefaultMode)
{
    UISizeSpec Result;
    Result.Mode = DefaultMode;

    if (Value.IsNumber())
    {
        Result.Mode = UISizeMode::Fixed;
        Result.Value = static_cast<float>(Value.GetDouble());
        return Result;
    }

    if (Value.IsString())
    {
        const std::string Text = Value.GetString();
        if (Text == "grow")
        {
            Result.Mode = UISizeMode::Grow;
        }
        else if (Text == "fit")
        {
            Result.Mode = UISizeMode::Fit;
        }
        else
        {
            Result.Mode = UISizeMode::Fit;
        }
    }

    return Result;
}

float ParseNumberOrDefault(const rapidjson::Value& Value, float DefaultValue)
{
    return Value.IsNumber() ? static_cast<float>(Value.GetDouble()) : DefaultValue;
}

csp::common::Vector3 ParseVector3(const rapidjson::Value& Value)
{
    csp::common::Vector3 Result(0.0f, 0.0f, 0.0f);

    if (!Value.IsObject())
    {
        return Result;
    }

    if (Value.HasMember("x"))
    {
        Result.X = ParseNumberOrDefault(Value["x"], Result.X);
    }
    if (Value.HasMember("y"))
    {
        Result.Y = ParseNumberOrDefault(Value["y"], Result.Y);
    }
    if (Value.HasMember("z"))
    {
        Result.Z = ParseNumberOrDefault(Value["z"], Result.Z);
    }

    return Result;
}

bool ParseBoolOrDefault(const rapidjson::Value& Value, bool DefaultValue) { return Value.IsBool() ? Value.GetBool() : DefaultValue; }

std::string ParseStringOrDefault(const rapidjson::Value& Value, const std::string& DefaultValue)
{
    return Value.IsString() ? std::string(Value.GetString()) : DefaultValue;
}

UIPadding ParsePadding(const rapidjson::Value& Value)
{
    UIPadding Padding;
    if (Value.IsNumber())
    {
        const float Uniform = static_cast<float>(Value.GetDouble());
        Padding.Left = Uniform;
        Padding.Right = Uniform;
        Padding.Top = Uniform;
        Padding.Bottom = Uniform;
        return Padding;
    }

    if (Value.IsObject())
    {
        if (Value.HasMember("left"))
        {
            Padding.Left = ParseNumberOrDefault(Value["left"], Padding.Left);
        }
        if (Value.HasMember("right"))
        {
            Padding.Right = ParseNumberOrDefault(Value["right"], Padding.Right);
        }
        if (Value.HasMember("top"))
        {
            Padding.Top = ParseNumberOrDefault(Value["top"], Padding.Top);
        }
        if (Value.HasMember("bottom"))
        {
            Padding.Bottom = ParseNumberOrDefault(Value["bottom"], Padding.Bottom);
        }
    }

    return Padding;
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
    return Left.Type == Right.Type && Left.Surface == Right.Surface && Left.TargetEntityId == Right.TargetEntityId
        && std::fabs(Left.SurfaceWidth - Right.SurfaceWidth) < 0.01f && std::fabs(Left.SurfaceHeight - Right.SurfaceHeight) < 0.01f
        && std::fabs(Left.WorldOffset.X - Right.WorldOffset.X) < 0.01f && std::fabs(Left.WorldOffset.Y - Right.WorldOffset.Y) < 0.01f
        && std::fabs(Left.WorldOffset.Z - Right.WorldOffset.Z) < 0.01f && Left.BillboardMode == Right.BillboardMode
        && std::fabs(Left.X - Right.X) < 0.01f && std::fabs(Left.Y - Right.Y) < 0.01f && std::fabs(Left.Width - Right.Width) < 0.01f
        && std::fabs(Left.Height - Right.Height) < 0.01f && std::fabs(Left.BackgroundColor.R - Right.BackgroundColor.R) < 0.001f
        && std::fabs(Left.BackgroundColor.G - Right.BackgroundColor.G) < 0.001f
        && std::fabs(Left.BackgroundColor.B - Right.BackgroundColor.B) < 0.001f
        && std::fabs(Left.BackgroundColor.A - Right.BackgroundColor.A) < 0.001f && std::fabs(Left.TextColor.R - Right.TextColor.R) < 0.001f
        && std::fabs(Left.TextColor.G - Right.TextColor.G) < 0.001f && std::fabs(Left.TextColor.B - Right.TextColor.B) < 0.001f
        && std::fabs(Left.TextColor.A - Right.TextColor.A) < 0.001f && std::fabs(Left.CornerRadius - Right.CornerRadius) < 0.01f
        && std::fabs(Left.Opacity - Right.Opacity) < 0.01f && Left.Text == Right.Text && std::fabs(Left.FontSize - Right.FontSize) < 0.01f
        && Left.AssetCollectionId == Right.AssetCollectionId && Left.ImageAssetId == Right.ImageAssetId && Left.HandlerId == Right.HandlerId
        && Left.Enabled == Right.Enabled;
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
        , ClayArenaMemory()
        , ClayContext(nullptr)
    {
    }

    csp::common::LogSystem& LogSystem;
    float ViewportWidth;
    float ViewportHeight;
    TextMeasureCallback MeasureCallback;
    bool HasWarnedFallbackMeasurement;
    std::vector<char> ClayArenaMemory;
    Clay_Context* ClayContext;
    std::map<std::string, UIEntityState> Entities;
    std::vector<UIPendingUpdate> PendingUpdates;

    static void ClayErrorThunk(Clay_ErrorData ErrorData)
    {
        Impl* Self = static_cast<Impl*>(ErrorData.userData);
        if (Self == nullptr)
        {
            return;
        }

        Self->LogSystem.LogMsg(csp::common::LogLevel::Error,
            std::string("NgxUIRuntime/Clay: ")
                .append(ErrorData.errorText.chars != nullptr ? ErrorData.errorText.chars : "",
                    static_cast<size_t>(std::max(0, ErrorData.errorText.length)))
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
    }

    static Clay_Dimensions MeasureTextThunk(Clay_StringSlice Text, Clay_TextElementConfig* Config, void* UserData)
    {
        Impl* Self = static_cast<Impl*>(UserData);
        Clay_Dimensions Dimensions;
        Dimensions.width = 0.0f;
        Dimensions.height = 0.0f;

        const float FontSize = Config != NULL && Config->fontSize > 0 ? static_cast<float>(Config->fontSize) : 16.0f;
        if (Self != NULL && Self->MeasureCallback)
        {
            csp::common::String Input(std::string(Text.chars, Text.length).c_str());
            const csp::common::Vector2 Size = Self->MeasureCallback(Input, FontSize);
            Dimensions.width = Size.X;
            Dimensions.height = Size.Y;
            return Dimensions;
        }

        if (Self != NULL && !Self->HasWarnedFallbackMeasurement)
        {
            Self->HasWarnedFallbackMeasurement = true;
            Self->LogSystem.LogMsg(csp::common::LogLevel::Warning, "NgxUIRuntime: Text measurement callback not set; using fallback text sizing.");
        }

        Dimensions.width = static_cast<float>(Text.length) * FontSize * 0.6f;
        Dimensions.height = FontSize * 1.2f;
        return Dimensions;
    }

    bool ParseNode(const rapidjson::Value& Value, const std::string& EntityId, UINode& OutNode)
    {
        if (!Value.IsObject() || !Value.HasMember("type") || !Value["type"].IsString())
        {
            return false;
        }

        const std::string Type = Value["type"].GetString();
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

        OutNode.Id = Value.HasMember("id") ? ParseStringOrDefault(Value["id"], "") : "";
        if (OutNode.Id.empty())
        {
            return false;
        }

        ApplyDefaultColors(OutNode);

        if (Value.HasMember("props") && Value["props"].IsObject())
        {
            const rapidjson::Value& Props = Value["props"];
            if (Props.HasMember("width"))
            {
                OutNode.Width = ParseSizeSpec(Props["width"], UISizeMode::Fit);
            }
            if (Props.HasMember("height"))
            {
                OutNode.Height = ParseSizeSpec(Props["height"], UISizeMode::Fit);
            }
            if (Props.HasMember("padding"))
            {
                OutNode.Padding = ParsePadding(Props["padding"]);
            }
            if (Props.HasMember("gap"))
            {
                OutNode.Gap = ParseNumberOrDefault(Props["gap"], OutNode.Gap);
                OutNode.RowGap = OutNode.Gap;
                OutNode.ColumnGap = OutNode.Gap;
            }
            if (Props.HasMember("rowGap"))
            {
                OutNode.RowGap = ParseNumberOrDefault(Props["rowGap"], OutNode.RowGap);
            }
            if (Props.HasMember("columnGap"))
            {
                OutNode.ColumnGap = ParseNumberOrDefault(Props["columnGap"], OutNode.ColumnGap);
            }
            if (Props.HasMember("margin"))
            {
                OutNode.Margin = ParseNumberOrDefault(Props["margin"], OutNode.Margin);
            }
            if (Props.HasMember("alignX"))
            {
                const std::string AlignValue = ParseStringOrDefault(Props["alignX"], "");
                if (OutNode.Kind == UIWidgetKind::ScreenRoot || OutNode.Kind == UIWidgetKind::WorldRoot)
                {
                    OutNode.SurfaceAlignX = AlignValue;
                }
                else
                {
                    OutNode.AlignX = AlignValue;
                }
            }
            if (Props.HasMember("alignY"))
            {
                const std::string AlignValue = ParseStringOrDefault(Props["alignY"], "");
                if (OutNode.Kind == UIWidgetKind::ScreenRoot || OutNode.Kind == UIWidgetKind::WorldRoot)
                {
                    OutNode.SurfaceAlignY = AlignValue;
                }
                else
                {
                    OutNode.AlignY = AlignValue;
                }
            }
            if (Props.HasMember("backgroundColor"))
            {
                OutNode.BackgroundColor = ParseColorString(ParseStringOrDefault(Props["backgroundColor"], ""));
            }
            if (Props.HasMember("textColor"))
            {
                OutNode.TextColor = ParseColorString(ParseStringOrDefault(Props["textColor"], ""));
            }
            else if (Props.HasMember("color"))
            {
                OutNode.TextColor = ParseColorString(ParseStringOrDefault(Props["color"], ""));
            }
            else if (Props.HasMember("colour"))
            {
                OutNode.TextColor = ParseColorString(ParseStringOrDefault(Props["colour"], ""));
            }
            if (Props.HasMember("opacity"))
            {
                OutNode.Opacity = ParseNumberOrDefault(Props["opacity"], OutNode.Opacity);
            }
            if (Props.HasMember("cornerRadius"))
            {
                OutNode.CornerRadius = ParseNumberOrDefault(Props["cornerRadius"], OutNode.CornerRadius);
            }
            if (Props.HasMember("visible"))
            {
                OutNode.Visible = ParseBoolOrDefault(Props["visible"], OutNode.Visible);
            }
            if (Props.HasMember("enabled"))
            {
                OutNode.Enabled = ParseBoolOrDefault(Props["enabled"], OutNode.Enabled);
            }
            if (Props.HasMember("fontSize"))
            {
                OutNode.FontSize = ParseNumberOrDefault(Props["fontSize"], OutNode.FontSize);
            }
            if (Props.HasMember("text"))
            {
                OutNode.Text = ParseStringOrDefault(Props["text"], "");
            }
            if (Props.HasMember("assetCollectionId"))
            {
                OutNode.AssetCollectionId = ParseStringOrDefault(Props["assetCollectionId"], "");
            }
            if (Props.HasMember("imageAssetId"))
            {
                OutNode.ImageAssetId = ParseStringOrDefault(Props["imageAssetId"], "");
            }
            if (Props.HasMember("onClickHandlerId"))
            {
                OutNode.HandlerId = ParseStringOrDefault(Props["onClickHandlerId"], "");
            }
            if (Props.HasMember("targetEntityId"))
            {
                OutNode.TargetEntityId = ParseStringOrDefault(Props["targetEntityId"], EntityId);
            }
            if (Props.HasMember("billboardMode"))
            {
                OutNode.BillboardMode = ParseStringOrDefault(Props["billboardMode"], "");
            }
            if (Props.HasMember("worldOffset"))
            {
                OutNode.WorldOffset = ParseVector3(Props["worldOffset"]);
            }
            if (Props.HasMember("attachTo"))
            {
                OutNode.FloatingAttachTo = ParseStringOrDefault(Props["attachTo"], OutNode.FloatingAttachTo);
            }
            if (Props.HasMember("attachX"))
            {
                OutNode.FloatingAttachX = ParseStringOrDefault(Props["attachX"], OutNode.FloatingAttachX);
            }
            if (Props.HasMember("attachY"))
            {
                OutNode.FloatingAttachY = ParseStringOrDefault(Props["attachY"], OutNode.FloatingAttachY);
            }
            if (Props.HasMember("offset") && Props["offset"].IsObject())
            {
                const rapidjson::Value& Offset = Props["offset"];
                if (Offset.HasMember("x"))
                {
                    OutNode.FloatingOffsetX = ParseNumberOrDefault(Offset["x"], OutNode.FloatingOffsetX);
                }
                if (Offset.HasMember("y"))
                {
                    OutNode.FloatingOffsetY = ParseNumberOrDefault(Offset["y"], OutNode.FloatingOffsetY);
                }
            }
            if (Props.HasMember("offsetX"))
            {
                OutNode.FloatingOffsetX = ParseNumberOrDefault(Props["offsetX"], OutNode.FloatingOffsetX);
            }
            if (Props.HasMember("offsetY"))
            {
                OutNode.FloatingOffsetY = ParseNumberOrDefault(Props["offsetY"], OutNode.FloatingOffsetY);
            }
            if (Props.HasMember("expand") && Props["expand"].IsObject())
            {
                const rapidjson::Value& Expand = Props["expand"];
                if (Expand.HasMember("width"))
                {
                    OutNode.FloatingExpandWidth = ParseNumberOrDefault(Expand["width"], OutNode.FloatingExpandWidth);
                }
                if (Expand.HasMember("height"))
                {
                    OutNode.FloatingExpandHeight = ParseNumberOrDefault(Expand["height"], OutNode.FloatingExpandHeight);
                }
            }
            if (Props.HasMember("expandWidth"))
            {
                OutNode.FloatingExpandWidth = ParseNumberOrDefault(Props["expandWidth"], OutNode.FloatingExpandWidth);
            }
            if (Props.HasMember("expandHeight"))
            {
                OutNode.FloatingExpandHeight = ParseNumberOrDefault(Props["expandHeight"], OutNode.FloatingExpandHeight);
            }
            if (Props.HasMember("zIndex"))
            {
                OutNode.FloatingZIndex = static_cast<int16_t>(ParseNumberOrDefault(Props["zIndex"], static_cast<float>(OutNode.FloatingZIndex)));
            }
            if (Props.HasMember("clipToParent"))
            {
                OutNode.FloatingClipToParent = ParseBoolOrDefault(Props["clipToParent"], OutNode.FloatingClipToParent);
            }
            if (Props.HasMember("pointerPassthrough"))
            {
                OutNode.FloatingPointerPassthrough = ParseBoolOrDefault(Props["pointerPassthrough"], OutNode.FloatingPointerPassthrough);
            }
        }

        if (OutNode.Kind == UIWidgetKind::WorldRoot && OutNode.TargetEntityId.empty())
        {
            OutNode.TargetEntityId = EntityId;
        }

        if (Value.HasMember("children") && Value["children"].IsArray())
        {
            const rapidjson::Value& Children = Value["children"];
            for (rapidjson::SizeType Index = 0; Index < Children.Size(); ++Index)
            {
                UINode Child;
                Child.Surface = OutNode.Surface;
                if (!ParseNode(Children[Index], EntityId, Child))
                {
                    continue;
                }
                Child.Surface = OutNode.Surface;
                OutNode.Children.push_back(Child);
            }
        }

        return true;
    }

    float MeasureTextWidthForNode(const std::string& Value, float FontSize)
    {
        Clay_TextElementConfig Config = CLAY__DEFAULT_STRUCT;
        Config.fontSize = static_cast<uint16_t>(std::max(1.0f, FontSize));
        const Clay_Dimensions Dimensions
            = MeasureTextThunk(Clay_StringSlice { static_cast<int32_t>(Value.size()), Value.c_str(), Value.c_str() }, &Config, this);
        return Dimensions.width;
    }

    float ResolveNodeOuterWidth(const UINode& Node, float AvailableWidth)
    {
        if (Node.Width.Mode == UISizeMode::Fixed)
        {
            return Node.Width.Value;
        }

        if (Node.Width.Mode == UISizeMode::Grow)
        {
            return AvailableWidth;
        }

        switch (Node.Kind)
        {
        case UIWidgetKind::Text:
            return MeasureTextWidthForNode(Node.Text, Node.FontSize);
        case UIWidgetKind::Button:
            return MeasureTextWidthForNode(Node.Text, Node.FontSize) + Node.Padding.Left + Node.Padding.Right;
        case UIWidgetKind::Spacer:
            return 0.0f;
        default:
            return AvailableWidth;
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
        const std::string& DrawableId, UIWidgetKind WidgetKindOverride)
    {
        MetadataStorage.emplace_back(std::make_unique<ClayNodeMetadata>());
        ClayNodeMetadata& Meta = *MetadataStorage.back();
        Meta.Id = DrawableId;
        Meta.EntityId = EntityId;
        Meta.Surface = Node.Surface == UISurfaceKind::World ? "world" : "screen";
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
        Meta.AssetCollectionId = Node.AssetCollectionId;
        Meta.ImageAssetId = Node.ImageAssetId;
        Meta.HandlerId = Node.HandlerId;
        Meta.Enabled = Node.Enabled;
        return &Meta;
    }

    Clay_TextElementConfig* CreateTextConfig(std::vector<std::unique_ptr<Clay_TextElementConfig>>& TextConfigStorage, const UINode& Node)
    {
        TextConfigStorage.emplace_back(std::make_unique<Clay_TextElementConfig>());
        Clay_TextElementConfig& TextConfig = *TextConfigStorage.back();
        std::memset(&TextConfig, 0, sizeof(TextConfig));
        TextConfig.fontSize = static_cast<uint16_t>(std::max(1.0f, Node.FontSize));
        TextConfig.lineHeight = static_cast<uint16_t>(std::max(1.0f, Node.FontSize * 1.2f));
        TextConfig.textColor = ToClayColor(ApplyOpacity(Node.TextColor, Node.Opacity));
        return &TextConfig;
    }

    void EmitButtonLabel(const std::string& EntityId, const UINode& Node, std::vector<std::unique_ptr<ClayNodeMetadata>>& MetadataStorage,
        std::vector<std::unique_ptr<Clay_TextElementConfig>>& TextConfigStorage)
    {
        Clay_TextElementConfig* TextConfig = CreateTextConfig(TextConfigStorage, Node);
        TextConfig->userData = CreateMetadata(MetadataStorage, EntityId, Node, Node.Id + "__label", UIWidgetKind::Text);
        Clay__OpenTextElement(ToClayString(Node.Text), TextConfig);
    }

    void EmitStandaloneText(const std::string& EntityId, const UINode& Node, std::vector<std::unique_ptr<ClayNodeMetadata>>& MetadataStorage,
        std::vector<std::unique_ptr<Clay_TextElementConfig>>& TextConfigStorage)
    {
        Clay_LayoutConfig LayoutConfig;
        std::memset(&LayoutConfig, 0, sizeof(LayoutConfig));
        LayoutConfig.sizing = ToClaySizing(Node.Width, Node.Height);
        LayoutConfig.padding.left = Node.Padding.Left;
        LayoutConfig.padding.right = Node.Padding.Right;
        LayoutConfig.padding.top = Node.Padding.Top;
        LayoutConfig.padding.bottom = Node.Padding.Bottom;
        LayoutConfig.childAlignment.x = ParseAlignX(Node.AlignX);
        LayoutConfig.childAlignment.y = ParseAlignY(Node.AlignY);

        Clay_ElementDeclaration Declaration = CLAY__DEFAULT_STRUCT;
        Declaration.layout = LayoutConfig;
        Declaration.userData = CreateMetadata(MetadataStorage, EntityId, Node, Node.Id + "__host", UIWidgetKind::TextHost);

        Clay__OpenElementWithId(Clay_GetElementId(ToClayString(Node.Id + "__host")));
        Clay__ConfigureOpenElement(Declaration);

        Clay_TextElementConfig* TextConfig = CreateTextConfig(TextConfigStorage, Node);
        TextConfig->userData = CreateMetadata(MetadataStorage, EntityId, Node, Node.Id, UIWidgetKind::Text);
        Clay__OpenTextElement(ToClayString(Node.Text), TextConfig);

        Clay__CloseElement();
    }

    void EmitNodeToClay(const std::string& EntityId, const UINode& Node, std::vector<std::unique_ptr<ClayNodeMetadata>>& MetadataStorage,
        std::vector<std::unique_ptr<Clay_TextElementConfig>>& TextConfigStorage)
    {
        if (!Node.Visible)
        {
            return;
        }

        if (Node.Kind == UIWidgetKind::Text)
        {
            EmitStandaloneText(EntityId, Node, MetadataStorage, TextConfigStorage);
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
            CustomConfig.customData = const_cast<char*>(Node.HandlerId.c_str());
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
        Declaration.userData = CreateMetadata(MetadataStorage, EntityId, Node, Node.Id, Node.Kind);

        Clay__OpenElementWithId(Clay_GetElementId(ToClayString(Node.Id)));
        Clay__ConfigureOpenElement(Declaration);

        if (Node.Kind == UIWidgetKind::Button)
        {
            EmitButtonLabel(EntityId, Node, MetadataStorage, TextConfigStorage);
        }
        else
        {
            for (size_t Index = 0; Index < Node.Children.size(); ++Index)
            {
                EmitNodeToClay(EntityId, Node.Children[Index], MetadataStorage, TextConfigStorage);
            }
        }

        Clay__CloseElement();
    }

    std::map<std::string, UIDrawable> BuildDrawables(const std::string& EntityId, const UINode& Root)
    {
        std::map<std::string, UIDrawable> Drawables;
        std::vector<std::unique_ptr<ClayNodeMetadata>> MetadataStorage;
        std::vector<std::unique_ptr<Clay_TextElementConfig>> TextConfigStorage;
        EnsureClayContext();

        const float SurfaceWidth = Root.Surface == UISurfaceKind::Screen
            ? ViewportWidth
            : (Root.Width.Mode == UISizeMode::Fixed && Root.Width.Value > 0.0f ? Root.Width.Value : 1024.0f);
        const float SurfaceHeight = Root.Surface == UISurfaceKind::Screen
            ? ViewportHeight
            : (Root.Height.Mode == UISizeMode::Fixed && Root.Height.Value > 0.0f ? Root.Height.Value : 768.0f);

        Clay_SetLayoutDimensions(Clay_Dimensions { SurfaceWidth, SurfaceHeight });
        Clay_BeginLayout();

        UINode SurfaceRoot = Root;
        SurfaceRoot.Width.Mode = UISizeMode::Fixed;
        SurfaceRoot.Width.Value = SurfaceWidth;
        SurfaceRoot.Height.Mode = UISizeMode::Fixed;
        SurfaceRoot.Height.Value = SurfaceHeight;

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
        Host.Children.push_back(SurfaceRoot);

        EmitNodeToClay(EntityId, Host, MetadataStorage, TextConfigStorage);

        const Clay_RenderCommandArray RenderCommands = Clay_EndLayout();
        for (int32_t Index = 0; Index < RenderCommands.length; ++Index)
        {
            Clay_RenderCommand* Command = Clay_RenderCommandArray_Get(const_cast<Clay_RenderCommandArray*>(&RenderCommands), Index);
            if (Command == nullptr || Command->userData == nullptr)
            {
                continue;
            }

            const ClayNodeMetadata& Meta = *static_cast<const ClayNodeMetadata*>(Command->userData);
            if (Meta.Id == Host.Id)
            {
                continue;
            }
            if (Meta.WidgetKind == UIWidgetKind::TextHost)
            {
                continue;
            }

            UIDrawable Drawable;
            Drawable.Id = Meta.Id;
            Drawable.Type = WidgetKindToString(Meta.WidgetKind);
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
            Drawable.AssetCollectionId = Meta.AssetCollectionId;
            Drawable.ImageAssetId = Meta.ImageAssetId;
            Drawable.HandlerId = Meta.HandlerId;
            Drawable.Enabled = Meta.Enabled;

            if (Command->commandType == CLAY_RENDER_COMMAND_TYPE_CUSTOM)
            {
                Drawable.Type = Meta.WidgetKind == UIWidgetKind::Button ? "button" : WidgetKindToString(Meta.WidgetKind);
            }
            else if (Command->commandType == CLAY_RENDER_COMMAND_TYPE_TEXT)
            {
                Drawable.Type = "text";
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
                        << "'. Existing type=" << Existing->second.Type << " new type=" << Drawable.Type << " existing text='"
                        << Existing->second.Text << "' new text='" << Drawable.Text
                        << "' commandType=" << RenderCommandTypeToString(Command->commandType)
                        << " metaWidgetKind=" << WidgetKindToString(Meta.WidgetKind) << " x=" << Drawable.X << " y=" << Drawable.Y
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
        Value.AddMember("assetCollectionId", rapidjson::Value(Drawable.AssetCollectionId.c_str(), Allocator), Allocator);
        Value.AddMember("imageAssetId", rapidjson::Value(Drawable.ImageAssetId.c_str(), Allocator), Allocator);
        Value.AddMember("handlerId", rapidjson::Value(Drawable.HandlerId.c_str(), Allocator), Allocator);
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
}

void NgxUIRuntime::SetViewportSize(float Width, float Height)
{
    if (!std::isfinite(Width) || !std::isfinite(Height) || Width <= 0.0f || Height <= 0.0f)
    {
        return;
    }

    Pimpl->ViewportWidth = Width;
    Pimpl->ViewportHeight = Height;

    for (std::map<std::string, UIEntityState>::iterator It = Pimpl->Entities.begin(); It != Pimpl->Entities.end(); ++It)
    {
        Mount(It->first, It->second.TreeJson);
    }
}

void NgxUIRuntime::SetTextMeasureCallback(TextMeasureCallback InCallback)
{
    Pimpl->MeasureCallback = InCallback;
    Pimpl->HasWarnedFallbackMeasurement = false;
}

bool NgxUIRuntime::Mount(const std::string& EntityId, const std::string& TreeJson)
{
    rapidjson::Document Document;
    Document.Parse(TreeJson.c_str());
    if (Document.HasParseError())
    {
        return false;
    }

    UINode Root;
    if (!Pimpl->ParseNode(Document, EntityId, Root))
    {
        return false;
    }

    const float RootAvailableWidth = Root.Surface == UISurfaceKind::Screen
        ? Pimpl->ViewportWidth
        : (Root.Width.Mode == UISizeMode::Fixed && Root.Width.Value > 0.0f ? Root.Width.Value : 1024.0f);
    Root = Pimpl->ExpandFlowRows(Root, RootAvailableWidth);

    const std::map<std::string, UIDrawable> NextDrawables = Pimpl->BuildDrawables(EntityId, Root);
    UIEntityState& State = Pimpl->Entities[EntityId];
    Pimpl->QueueDiff(EntityId, State.Drawables, NextDrawables);
    State.TreeJson = TreeJson;
    State.Drawables = NextDrawables;
    return true;
}

bool NgxUIRuntime::Unmount(const std::string& EntityId)
{
    const std::map<std::string, UIEntityState>::iterator It = Pimpl->Entities.find(EntityId);
    if (It == Pimpl->Entities.end())
    {
        return false;
    }

    Pimpl->QueueDiff(EntityId, It->second.Drawables, std::map<std::string, UIDrawable>());
    Pimpl->Entities.erase(It);
    return true;
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
