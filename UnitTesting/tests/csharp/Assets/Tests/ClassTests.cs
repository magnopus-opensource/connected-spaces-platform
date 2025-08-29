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

using NUnit.Framework;

namespace Csp.Tests
{
    public class ClassTests
    {
        [Test]
        public void CreateAndDestroySimpleClass()
        {
            // Calling the constructor creates an instance of the native class.
            var simpleClass = new SimpleClass();
            Assert.IsTrue(simpleClass.PointerIsValid);

            // Verify the internal pointer is set.
            // Note that we have to use reflection here to access the internal field.
            var ptrField = typeof(SimpleClass).GetField("_ptr", System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance);
            var ptrValue = ptrField.GetValue(simpleClass);
            Assert.IsNotNull(ptrValue);

            // Verify that the instance owns the native pointer.
            // Note that we have to use reflection here to access the internal field.
            var ownsPtrField = typeof(SimpleClass).GetField("_ownsPtr", System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance);
            var ownsPtrValue = ownsPtrField.GetValue(simpleClass);
            Assert.IsTrue((bool)ownsPtrValue);

            // Calling Dispose will call the destructor on the native class and
            // mark this instance as disposed.
            simpleClass.Dispose();

            // Once Dispose is called, the internal pointer is no longer valid.
            // However, Dispose does not null out the pointer so the class is
            // left in an indeterminate state.
            // There is also no way to query whether Dispose has been called.
            Assert.IsTrue(simpleClass.PointerIsValid);

            // Verify the internal pointer is still set.
            var ptrValueDisposed = ptrField.GetValue(simpleClass);
            Assert.IsNotNull(ptrValueDisposed);
        }

        [Test]
        public void MultipleDisposes()
        {
            var simpleClass = new SimpleClass();

            // Call Dispose multiple times and ensure no errors occur.
            simpleClass.Dispose();
            simpleClass.Dispose();
        }
    }
}