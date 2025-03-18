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


namespace Csp
{
    /// <summary>
    /// A type-safe wrapper around a pointer to an unmanaged type
    /// </summary>
    /// <typeparam name="T">The type of the object the pointer refers to</typeparam>
    public struct Ref<T> where T : unmanaged
    {
        internal IntPtr Pointer;

        internal Ref(IntPtr Pointer)
        {
            this.Pointer = Pointer;
        }

        public static implicit operator T(Ref<T> value) => value.Value;

        public unsafe T Value {
            get => *(T*)Pointer.ToPointer();
            set => *(T*)Pointer.ToPointer() = value;
        }

        public bool IsValid() => Pointer != IntPtr.Zero;
    }
}
