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

    public abstract class NativeClassWrapper
    {
        internal IntPtr _ptr;
        internal bool _ownsPtr;
        internal bool _disposed = false;

        /// <summary>
        /// Fetch the safe type name for a given type that has the NativeClassWrapperAttribute.
        /// This is currently used by templated types (e.g. `Csp.Common.Array`) to find the corresponding
        /// native methods through reflection.
        /// </summary>
        /// <param name="type"></param>
        /// <returns></returns>
        /// <exception cref="ArgumentNullException">Thrown when the type is null.</exception>
        /// <exception cref="InvalidOperationException">Thrown when the type does not have a valid NativeClassWrapperAttribute.</exception>
        internal static string GetSafeTypeName(Type type)
        {
            if (type == null)
            {
                throw new ArgumentNullException(nameof(type));
            }

            var attr = (NativeClassWrapperAttribute)Attribute.GetCustomAttribute(type, typeof(NativeClassWrapperAttribute));
            if (attr == null || string.IsNullOrEmpty(attr.SafeName))
            {
                throw new InvalidOperationException($"Type {type.Name} does not have a NativeClassWrapperAttribute.");
            }

            if (attr.Template)
            {
                throw new NotImplementedException($"Templated type {type.Name} is not supported through runtime reflection.");
            }

            return attr?.SafeName ?? throw new InvalidOperationException($"Type {type.Name} does not have a NativeClassAttribute with a SafeName.");
        }

        [Obsolete("NativeClassWrapper instances are now guaranteed to be valid")]
        public bool PointerIsValid => _ptr != IntPtr.Zero;

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

    /// <summary>
    /// Used to decorate generated NativeClassWrapper classes to provide a way to look up the native type name.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, Inherited = false)]
    public class NativeClassWrapperAttribute : Attribute
    {
        public string SafeName { get; }

        public bool Template { get; }

        public NativeClassWrapperAttribute(string safeName)
        {
            SafeName = safeName;
            Template = false;
        }

        public NativeClassWrapperAttribute(string safeName, bool template)
        {
            SafeName = safeName;
            Template = template;
        }
    }
}
