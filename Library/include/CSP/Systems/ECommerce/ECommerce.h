/*
* Copyright 2023 Magnopus LLC

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
#pragma once
#include "CSP/CSPCommon.h"
#include "CSP/Common/Map.h"
#include "CSP/Common/String.h"
#include "CSP/Systems/WebService.h"

namespace csp::services
{

CSP_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
CSP_END_IGNORE

} // namespace csp::services

namespace csp::systems
{
/// @ingroup ECommerce System
/// @brief Represents currency information for a product
class CSP_API CurrencyInfo
{
public:
    CurrencyInfo()
        : Amount(0.0)
    {
    }

    /// @brief Currency Amount.
    double Amount;
    /// @brief Currency type
    csp::common::String CurrencyCode;

    bool operator==(const CurrencyInfo& other) const;
    bool operator!=(const CurrencyInfo& other) const;
};

/// @ingroup ECommerce System
/// @brief Represents media information for a product
class CSP_API ProductMediaInfo
{
public:
    ProductMediaInfo()
        : Width(0)
        , Height(0)
    {
    }

    /// @brief Type of media content used.
    csp::common::String MediaContentType;
    /// @brief Alternative description of the media.
    csp::common::String Alt;
    /// @brief Url of the media.
    csp::common::String Url;
    /// @brief Width of the media.
    int32_t Width;
    /// @brief Height of the media.
    int32_t Height;

    bool operator==(const ProductMediaInfo& other) const;
    bool operator!=(const ProductMediaInfo& other) const;
};

/// @ingroup ECommerce System
/// @brief Represents option for a variant
class CSP_API VariantOptionInfo
{
public:
    VariantOptionInfo() = default;

    /// @brief Id of the variant option.
    csp::common::String Name;
    /// @brief Value of variant option.
    csp::common::String Value;

    bool operator==(const VariantOptionInfo& other) const;
    bool operator!=(const VariantOptionInfo& other) const;
};

/// @ingroup ECommerce System
/// @brief Represents variant information for a product
class CSP_API ProductVariantInfo
{
public:
    ProductVariantInfo()
        : AvailableForSale(false)
        , AvailableStock(0)
    {
    }

    /// @brief Id of the variant.
    csp::common::String Id;
    /// @brief Title of the variant.
    csp::common::String Title;
    /// @brief Url of variant.
    csp::common::String Url;
    /// @brief Is variant available for sale.
    bool AvailableForSale;
    /// @brief Media for a variant
    ProductMediaInfo Media;
    /// @brief Additional options for variant
    csp::common::Array<VariantOptionInfo> Options;
    /// @brief Unit price for the variant
    CurrencyInfo UnitPrice;
    /// @brief Quantity of variant available
    int32_t AvailableStock;

    bool operator==(const ProductVariantInfo& other) const;
    bool operator!=(const ProductVariantInfo& other) const;
};

/// @ingroup ECommerce System
/// @brief Represents a single product and the information associated with it.
class CSP_API ProductInfo
{
public:
    ProductInfo() = default;

    /// @brief Id of the product.
    csp::common::String Id;
    /// @brief Title of the product.
    csp::common::String Title;
    /// @brief Time the product was created.
    csp::common::String CreatedAt;
    /// @brief Description of the product.
    csp::common::String Description;
    /// @brief Array of product variants.
    csp::common::Array<ProductVariantInfo> Variants;
    /// @brief Array of product tags.
    csp::common::Array<csp::common::String> Tags;
    /// @brief This array holds media for the product
    csp::common::Array<ProductMediaInfo> Media;

    bool operator==(const ProductInfo& other) const;
    bool operator!=(const ProductInfo& other) const;
};

/// @ingroup ECommerce System
/// @brief Represents Checkout information for a product
class CSP_API CheckoutInfo
{
public:
    CheckoutInfo() = default;

    /// @brief Url of the Store.
    csp::common::String StoreUrl;
    /// @brief Url of Checkout.
    csp::common::String CheckoutUrl;

    bool operator==(const CheckoutInfo& other) const;
    bool operator!=(const CheckoutInfo& other) const;
};

/// @ingroup ECommerce System
/// @brief Represents a line in a cart.
class CSP_API CartLine
{
public:
    CartLine()
        : Quantity(0)
    {
    }

    /// @brief ID of the line in the cart.
    csp::common::String CartLineId;

    /// @brief ID of the variant of the product.
    csp::common::String ProductVariantId;

    /// @brief Quantity of the product in the cart.
    int Quantity;

    bool operator==(const CartLine& other) const;
    bool operator!=(const CartLine& other) const;
};

/// @ingroup ECommerce System
/// @brief Represents a cart.
class CSP_API CartInfo
{
public:
    CartInfo()
        : TotalQuantity(0)
    {
    }

    /// @brief Space that the cart is associated with.
    csp::common::String SpaceId;

    /// @brief ID of the cart.
    csp::common::String CartId;

    /// @brief An array of the lines in the cart.
    csp::common::Array<CartLine> CartLines;

    /// @brief Total quantity of all lines in the cart.
    int TotalQuantity;

    bool operator==(const CartInfo& other) const;
    bool operator!=(const CartInfo& other) const;
};

/// @ingroup ECommerce System
/// @brief Represents a shopify store.
class CSP_API ShopifyStoreInfo
{
public:
    ShopifyStoreInfo()
        : IsEcommerceActive(false)
    {
    }

    /// @brief ID of the store.
    csp::common::String StoreId;

    /// @brief Name of the store.
    csp::common::String StoreName;

    /// @brief ID of the store owner.
    csp::common::String SpaceOwnerId;

    /// @brief Space that the cart is associated with.
    csp::common::String SpaceId;

    /// @brief Is Ecommerce active.
    bool IsEcommerceActive;

    bool operator==(const ShopifyStoreInfo& other) const;
    bool operator!=(const ShopifyStoreInfo& other) const;
};

/// @ingroup ECommerce System
/// @brief Data class used to contain information when attempting to get Product Info.
class CSP_API ProductInfoResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the Product Info being stored.
    /// @return ProductInfo : reference to the ProductInfo
    const ProductInfo& GetProductInfo() const;

    /// @brief Retrieves the Product Info being stored.
    /// @return ProductInfo : reference to the ProductInfo
    ProductInfo& GetProductInfo();

private:
    ProductInfoResult(void*) {};

    CSP_NO_EXPORT void OnResponse(const csp::services::ApiResponseBase* apiResponse) override;

    ProductInfo m_productInformation;
};

/// @ingroup ECommerce System
/// @brief Data class used to contain information when attempting to get Arrays of Product Info.
class CSP_API ProductInfoCollectionResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the Product Info Array being stored.
    /// @return csp::common::Array<ProductInfo> : reference to the ProductInfos
    const csp::common::Array<ProductInfo>& GetProducts() const;

    /// @brief Retrieves the Product Info Array being stored.
    /// @return csp::common::Array<ProductInfo> : reference to the ProductInfos
    csp::common::Array<ProductInfo>& GetProducts();

private:
    ProductInfoCollectionResult(void*) {};

    CSP_NO_EXPORT void OnResponse(const csp::services::ApiResponseBase* apiResponse) override;

    csp::common::Array<ProductInfo> m_products;
};

/// @ingroup ECommerce System
/// @brief Data class used to contain information when attempting to get Checkout Info.
class CSP_API CheckoutInfoResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the Checkout Info being stored.
    /// @return ProductInfo : reference to the CheckoutInfo
    const CheckoutInfo& GetCheckoutInfo() const;

    /// @brief Retrieves the Checkout Info being stored.
    /// @return ProductInfo : reference to the CheckoutInfo
    CheckoutInfo& GetCheckoutInfo();

    CSP_NO_EXPORT CheckoutInfoResult(csp::systems::EResultCode resCode, uint16_t httpResCode)
        : csp::systems::ResultBase(resCode, httpResCode) {};

private:
    CheckoutInfoResult(void*) {};

    CSP_NO_EXPORT void OnResponse(const csp::services::ApiResponseBase* apiResponse) override;

    CheckoutInfo m_checkoutInformation;
};

/// @ingroup ECommerce System
/// @brief Data class used to contain information when attempting to get a Cart.
class CSP_API CartInfoResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class ECommerceSystem;

    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the Cart Info being stored.
    /// @return ProductInfo : reference to the CartInfo
    const CartInfo& GetCartInfo() const;

    /// @brief Retrieves the Cart Info being stored.
    /// @return ProductInfo : reference to the CartInfo
    CartInfo& GetCartInfo();

    CSP_NO_EXPORT CartInfoResult(csp::systems::EResultCode resCode, uint16_t httpResCode)
        : csp::systems::ResultBase(resCode, httpResCode) {};

private:
    CartInfoResult(void*) {};

    CSP_NO_EXPORT void OnResponse(const csp::services::ApiResponseBase* apiResponse) override;

    CartInfo m_cart;
};

class CSP_API AddShopifyStoreResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    const ShopifyStoreInfo& GetShopifyStoreInfo() const;

    ShopifyStoreInfo& GetShopifyStoreInfo();

private:
    AddShopifyStoreResult(void*) {};

    CSP_NO_EXPORT void OnResponse(const csp::services::ApiResponseBase* apiResponse) override;

    ShopifyStoreInfo m_store;
};

class CSP_API GetShopifyStoresResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the ShopifyStoreInfo Array being stored.
    /// @return csp::common::Array<ShopifyStoreInfo> : reference to the ShopifyStoreInfos
    const csp::common::Array<ShopifyStoreInfo>& GetShopifyStores() const;

    /// @brief Retrieves the ShopifyStoreInfo Array being stored.
    /// @return csp::common::Array<ShopifyStoreInfo> : reference to the ShopifyStoreInfos
    csp::common::Array<ShopifyStoreInfo>& GetShopifyStores();

private:
    GetShopifyStoresResult(void*) {};

    CSP_NO_EXPORT void OnResponse(const csp::services::ApiResponseBase* apiResponse) override;

    csp::common::Array<ShopifyStoreInfo> m_stores;
};

class CSP_API ValidateShopifyStoreResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
    CSP_END_IGNORE
    /** @endcond */

public:
    bool ValidateResult;

private:
    ValidateShopifyStoreResult(void*) {};

    CSP_NO_EXPORT void OnResponse(const csp::services::ApiResponseBase* apiResponse) override;
};

typedef std::function<void(const ProductInfoResult& result)> ProductInfoResultCallback;

typedef std::function<void(const ProductInfoCollectionResult& result)> ProductInfoCollectionResultCallback;

typedef std::function<void(const CheckoutInfoResult& result)> CheckoutInfoResultCallback;

typedef std::function<void(const CartInfoResult& result)> CartInfoResultCallback;

typedef std::function<void(const AddShopifyStoreResult& result)> AddShopifyStoreResultCallback;

typedef std::function<void(const AddShopifyStoreResult& result)> SetECommerceActiveResultCallback;

typedef std::function<void(const ValidateShopifyStoreResult& result)> ValidateShopifyStoreResultCallback;

typedef std::function<void(const GetShopifyStoresResult& result)> GetShopifyStoresResultCallback;

} // namespace csp::systems
