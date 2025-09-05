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
        private IntPtr _ptrValue = IntPtr.Zero;
        internal bool _ownsPtr;
        internal bool _disposed = false;

        internal virtual string _safeTypeName { get; }

        public bool PointerIsValid => _ptrValue != IntPtr.Zero;

        /// <summary>
        /// Pointer to the native object.
        /// </summary>
        /// <exception cref="NullReferenceException">Thrown if the pointer is null.</exception>
        /// <exception cref="InvalidOperationException">Thrown if attempting to change the pointer once it's set to a non-null value.</exception>
        internal IntPtr _ptr
        {
            get
            {
                // Prevent accessing the pointer if it's null
                if (_ptrValue == IntPtr.Zero)
                {
                    throw new NullReferenceException($"Attempting to access a null pointer for {_safeTypeName}");
                }
                return _ptrValue;
            }
            set
            {
                // Prevent changing the pointer once it's set to a non-null value
                if (_ptrValue != IntPtr.Zero && value != IntPtr.Zero)
                {
                    throw new InvalidOperationException($"Attempting to change the native pointer for {_safeTypeName} from {_ptrValue} to {value}");
                }

                _ptrValue = value;
            }
        }

        public NativeClassWrapper() { }

        internal NativeClassWrapper(NativePointer ptr)
        {
            _ptr = ptr.Pointer;
            _ownsPtr = ptr.OwnsOwnData;
        }
    }
}
