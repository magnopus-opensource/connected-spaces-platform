/*
 * Copyright 2025 Magnopus LLC

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

using System;
using System.Runtime.InteropServices;


namespace Csp
{
    [StructLayout(LayoutKind.Sequential)]
    internal struct NativePointer
    {
        public IntPtr Pointer;
        readonly byte _ownsOwnData;

        public bool OwnsOwnData => _ownsOwnData != 0;

        internal NativePointer(IntPtr pointer, byte ownsOwnData)
        {
            Pointer = pointer;
            _ownsOwnData = ownsOwnData;
        }

        public static NativePointer Zero => new NativePointer(IntPtr.Zero, 0);
    }

    public class NativeClassWrapper
    {
        internal IntPtr _ptr;
        internal bool _ownsPtr;
        internal bool _disposed = false;

        internal virtual string _safeTypeName { get; }

        [Obsolete("NativeClassWrapper instances are now guaranteed to be valid")]
        public bool PointerIsValid => _ptr != IntPtr.Zero;

        public NativeClassWrapper() { }

        internal NativeClassWrapper(NativePointer ptr)
        {
            if (ptr.Pointer == IntPtr.Zero)
            {
                throw new ArgumentException("NativePointer cannot be zero.", nameof(ptr));
            }

            _ptr = ptr.Pointer;
            _ownsPtr = ptr.OwnsOwnData;
        }
    }
}
