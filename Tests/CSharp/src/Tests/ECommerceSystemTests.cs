using System.Collections.Generic;
using System.IO;
using System.Linq;
using Common = Csp.Common;
using Systems = Csp.Systems;

using CSharpTests;
using static CSharpTests.TestHelper;


#nullable enable annotations


namespace CSPEngine
{
    static class ECommerceSystemTests
    {
        
        static Common::Map<string,string> getShopifyDetails()
        {
            if (!File.Exists("ShopifyCreds.txt"))
                LogFatal("ShopifyCreds.txt not found! This file must exist and must contain the following information:\nSpaceId <SpaceId>\nProductId <ProductId>");

            var creds = File.ReadAllText("ShopifyCreds.txt").Split(' ', '\n').Select(s => s.Trim()).ToArray();
            var OutMap = new Common.Map<string, string>();

            OutMap[creds[0]] = creds[1];
            OutMap[creds[2]] = creds[3];
            
            return OutMap;
        }
#if RUN_ECOMMERCE_TESTS || RUN_ECOMMERCE_GET_PRODUCT_INFORMATION_TEST
        [Test]
        public static void CreateTicketedEventActiveTrueTest()
        {
            GetFoundationSystems(out var userSystem, out _, out _, out _, out _, out _, out _, out _, out _, out var eCommerceSystem);

            // To use this test please follow end to end testing steps first:
            // https://docs.google.com/document/d/1D2fzF88c4NfPp26ciJHf-qelNFY5jqQHmt8lqolOmq0/edit?usp=sharing

            // This is an example from shopify dev quickstart "Gift Card"
            string ProductId				= "gid://shopify/Product/8660541047057";
            string ProductTitle			= "Gift Card";
            string ProductDescription	= "This is a gift card for the store";
            string ImageMediaContentType = "IMAGE";
            string ImageAlt				= "Gift card that shows text: Generated data gift card";
            string ImageUrl				= "https://cdn.shopify.com/s/files/1/0803/6070/2225/products/gift_card.png?v=1691076851";
            const int ImageWidth						= 2881;
            const int ImageHeight						= 2881;
            const ulong VariantSize						= 4UL;
            const ulong MediaSize							= 1UL;
            const ulong OptionsSize						= 1UL;
            string OptionsName			= "Denominations";
            Common::Array<string> VariantTitleAndOptionValue = new Common::Array<string>(4);
            VariantTitleAndOptionValue[0] = "$10";
            VariantTitleAndOptionValue[1] = "$25";
            VariantTitleAndOptionValue[2] = "$50";
            VariantTitleAndOptionValue[3] = "$100";
            Common::Array<string> VariantIds =new Common::Array<string>(4);
            VariantIds[0] = "gid://shopify/ProductVariant/46314311516433";
            VariantIds[1] = "gid://shopify/ProductVariant/46314311647505";
            VariantIds[2] = "gid://shopify/ProductVariant/46314311745809";
            VariantIds[3] = "gid://shopify/ProductVariant/46314311844113";
            // Log in
            _ = UserSystemTests.LogIn(userSystem);
            
            var details = getShopifyDetails();

            using var result = eCommerceSystem.GetProductInformation(details["SpaceId"], details["ProductId"]).Result;
            var resCode = result.GetResultCode();
            Assert.AreEqual(resCode, Services.EResultCode.Success);


            Assert.AreEqual(result.GetProductInfo().Id, ProductId);
			Assert.AreEqual(result.GetProductInfo().Title, ProductTitle);
			Assert.AreEqual(result.GetProductInfo().Description, ProductDescription);
			Assert.AreEqual(result.GetProductInfo().Tags.Size(), 0UL);

			Assert.AreEqual(result.GetProductInfo().Media.Size(), MediaSize);

			for (uint i = 0; i < result.GetProductInfo().Media.Size(); ++i)
			{
				Assert.AreEqual(result.GetProductInfo().Media[i].MediaContentType, ImageMediaContentType);
				Assert.AreEqual(result.GetProductInfo().Media[i].Url, ImageUrl);
				Assert.AreEqual(result.GetProductInfo().Media[i].Alt, ImageAlt);
				Assert.AreEqual(result.GetProductInfo().Media[i].Width, ImageWidth);
				Assert.AreEqual(result.GetProductInfo().Media[i].Height, ImageHeight);
			}
			Assert.AreEqual(result.GetProductInfo().Variants.Size(), VariantSize);

			for (uint i = 0; i < result.GetProductInfo().Variants.Size(); ++i)
			{
				Assert.AreEqual(result.GetProductInfo().Variants[i].Id, VariantIds[i]);
				Assert.AreEqual(result.GetProductInfo().Variants[i].Title, VariantTitleAndOptionValue[i]);
				Assert.AreEqual(result.GetProductInfo().Variants[i].AvailableForSale, true);
				Assert.AreEqual(result.GetProductInfo().Variants[i].Media.MediaContentType, "");
				Assert.AreEqual(result.GetProductInfo().Variants[i].Media.Alt, ImageAlt);
				Assert.AreEqual(result.GetProductInfo().Variants[i].Media.Url, ImageUrl);
				Assert.AreEqual(result.GetProductInfo().Variants[i].Media.Width, ImageWidth);
				Assert.AreEqual(result.GetProductInfo().Variants[i].Media.Height, ImageHeight);

				Assert.AreEqual(result.GetProductInfo().Variants[i].Options.Size(), OptionsSize);

				for (uint n = 0; n < result.GetProductInfo().Variants[i].Options.Size(); ++n)
				{
					Assert.AreEqual(result.GetProductInfo().Variants[i].Options[n].Name, OptionsName);
					Assert.AreEqual(result.GetProductInfo().Variants[i].Options[n].Value, VariantTitleAndOptionValue[i]);
				}

				Assert.AreEqual(result.GetProductInfo().Variants[i].UnitPrice.Amount, 0);
				Assert.AreEqual(result.GetProductInfo().Variants[i].UnitPrice.CurrencyCode, "");
			}

        }
#endif
    }
}