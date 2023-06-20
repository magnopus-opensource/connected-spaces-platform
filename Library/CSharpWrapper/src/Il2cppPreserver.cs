// ------------------------------------------------------------------
// Copyright (c) Magnopus. All Rights Reserved.
// ------------------------------------------------------------------

#if ENABLE_IL2CPP

using Csp.Multiplayer;

using UnityEngine.Scripting;


namespace Csp
{
    internal class Il2cppPreserver
    {
        /// <summary>
        /// Method to ensure that Foundation code is not stripped within IL2CPP builds.
        /// Note that as we have the <see cref="PreserveAttribute"/> this 
        /// method does not actually need to be called anywhere in order to prevent stripping.
        /// </summary>
        [Preserve]
        private void Preserver()
        {
            ComponentUpdateInfo componentUpdateInfoNative = new ComponentUpdateInfo(default(NativePointer));
        }
    }
}

#endif
