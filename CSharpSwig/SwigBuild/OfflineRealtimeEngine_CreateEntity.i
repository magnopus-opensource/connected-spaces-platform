%module ConnectedSpacesPlatform

%{
#include "CSP/Common/Vector.h"
#include "CSP/Multiplayer/SpaceTransform.h"
#include "CSP/Common/Interfaces/IRealtimeEngine.h"
#include "CSP/Multiplayer/OfflineRealtimeEngine.h"
%}

/* SWIG std helpers */
%include "typemaps.i"
%include "std_string.i"

/* Delegate shape SWIG will expose in C# */
%inline %{
  typedef void (*EntityCreatedCbFP)(void* UserContext, csp::multiplayer::SpaceEntity* entity);
%}


namespace csp {
  namespace multiplayer {
    class SpaceEntity; //Havn't defined it for this prototype, it'll just be an opaque handle

  }
}

/* Expose math & transform types with minimal surface (ctors + fields) */
namespace csp {
  namespace common {
    class Vector3 {
    public:
      Vector3();
      Vector3(float X, float Y, float Z);
      float X; float Y; float Z;
    };
    class Vector4 {
    public:
      Vector4();
      Vector4(float X, float Y, float Z, float W);
      float X; float Y; float Z; float W;
    };
  }
  namespace multiplayer {
    class SpaceTransform {
    public:
      SpaceTransform();
      SpaceTransform(const csp::common::Vector3& Position,
                     const csp::common::Vector4& Rotation,
                     const csp::common::Vector3& Scale);
      csp::common::Vector3 Position;
      csp::common::Vector4 Rotation;
      csp::common::Vector3 Scale;
    };
  }
}

/* Engine (now with default ctor you added) */
namespace csp {
  namespace common {
    class IRealtimeEngine {
        IRealtimeEngine() = delete;
    };
  }

  namespace multiplayer {
    class OfflineRealtimeEngine : public csp::common::IRealtimeEngine {
    public:
      OfflineRealtimeEngine();
    };
  }
}


/* Add an overload that adapts (fnptr + user) -> std::function */
%extend csp::common::IRealtimeEngine {
  void CreateEntity_Managed(const std::string& name,
                            const csp::multiplayer::SpaceTransform& xform,
                            std::optional<uint64_t> parent,
                            EntityCreatedCbFP cb,
                            void* UserContext)
  {

    $self->CreateEntity(
      name, xform, parent,
      [cb, UserContext](csp::multiplayer::SpaceEntity* e) {
        if (cb) cb(UserContext, e);
      });
  }
}