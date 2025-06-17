using NUnit.Framework;

namespace Csp.Tests
{
    class TemplateClassTests
    {
        [Test]
        public void Test_int()
        {
            // Arrange
            var templateClass = new TemplateTestClass<int>();

            // Act
            templateClass.SetValue(42);
            var value = templateClass[0];

            // Assert
            Assert.AreEqual(42, value);
        }
    }
}