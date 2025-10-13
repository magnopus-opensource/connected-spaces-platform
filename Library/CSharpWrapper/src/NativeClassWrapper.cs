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

    /// <summary>
    /// Base class for all CSP objects.
    /// Provides base memory management for derived interop types.
    /// </summary>
    public class NativeClassWrapper
    {
        /// <summary>
        /// The native C pointer to the object for use with P/Invoke methods.
        /// </summary>
        /// <remarks>
        /// This must be accessed through <see cref="_ptr"/> to avoid passing `nullptr` to
        /// member functions in native code.
        /// </remarks>
        private IntPtr _ptrValue = IntPtr.Zero;

        /// <summary>
        /// Cached value of <see cref="NativePointer.OwnsOwnData"/>.
        /// If true, this object owns the underlying pointer and is responsible for
        /// calling its destructor when disposed.
        /// </summary>
        internal bool _ownsPtr;

        /// <summary>
        /// Indicates whether the object has been disposed.
        /// Used to implement the <see cref="System.IDisposable"/> pattern in subclasses.
        /// </summary>
        internal bool _disposed = false;

        /// <summary>
        /// Gets the name of the name-mangled native type pointed to by <see cref="_ptrValue"/>.
        /// </summary>
        internal virtual string _safeTypeName { get; }

        /// <summary>
        /// Is the underlying pointer (<see cref="_ptr"/>) valid.
        /// </summary>
        /// <remarks>
        /// This can be used to prevent a <see cref="NullReferenceException"/> when calling <see cref="_ptr"/>.
        /// </remarks>
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
                // Prevent accessing the pointer if the object has been disposed
                if (_disposed)
                {
                    throw new ObjectDisposedException(_safeTypeName, $"Attempting to access a disposed instance of {_safeTypeName}");
                }

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

        /// <summary>
        /// Construct an empty instance of the object.
        /// </summary>
        /// <remarks>
        /// This is generally an invalid operation since it creates an object where
        /// <see cref="_ptr"/> is guaranteed to throw an exception.
        /// This empty constructor is currently required by generated generic types
        /// (such as <see cref="Csp.Common.List<T>"/>) which look up the underlying
        /// base type stored in <see cref="_safeTypeName"/>.
        /// </remarks>
        public NativeClassWrapper() { }

        /// <summary>
        /// Construct an instance of the object given a <see cref="NativePointer"/>
        /// from the native runtime.
        /// </summary>
        /// <param name="ptr">A valid <see cref="NativePointer"/> representing the runtime object.</param>
        internal NativeClassWrapper(NativePointer ptr)
        {
            _ptr = ptr.Pointer;
            _ownsPtr = ptr.OwnsOwnData;
        }
    }
}
