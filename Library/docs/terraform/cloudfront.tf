data "aws_cloudfront_origin_access_identity" "main" {
  id = var.aws_cloudfront_origin_access_identity_id
}

resource "aws_cloudfront_response_headers_policy" "main" {
  name = "${aws_s3_bucket.main.id}_response_headers_policy"

  cors_config {
    access_control_allow_credentials = false
    access_control_max_age_sec       = 600
    origin_override                  = false

    access_control_allow_headers {
      items = ["*"]
    }
    access_control_allow_methods {
      items = ["GET", "HEAD", "OPTIONS"]
    }
    access_control_allow_origins {
      items = ["*"]
    }
    access_control_expose_headers {
      items = ["*"]
    }
  }
  security_headers_config {
    frame_options {
      frame_option = "SAMEORIGIN"
      override     = false
    }
  }
  custom_headers_config {
    items {
      header   = "Cache-Control"
      value    = "max-age=604800"
      override = false
    }
    items {
      header   = "Cross-Origin-Embedder-Policy"
      value    = "require-corp"
      override = false
    }
    items {
      header   = "Cross-Origin-Opener-Policy"
      value    = "same-origin"
      override = false
    }
    items {
      header   = "Cross-Origin-Resource-Policy"
      value    = "same-origin"
      override = false
    }
  }
}

resource "aws_cloudfront_distribution" "main" {
  origin {
    domain_name = aws_s3_bucket.main.bucket_regional_domain_name
    origin_id   = "S3-${aws_s3_bucket.main.id}"

    s3_origin_config {
      origin_access_identity = "origin-access-identity/cloudfront/${data.aws_cloudfront_origin_access_identity.main.id}"
    }
  }

  enabled         = true
  is_ipv6_enabled = false # Using IPv4 only to mitigate VPN routing issues
  web_acl_id      = var.aws_wafv2_web_acl_arn

  aliases = concat(["${var.project_shortname}.${data.aws_route53_zone.canonical.name}"], var.alias_aws_public_zone_id != null ? [data.aws_route53_zone.alias[0].name] : [])

  default_cache_behavior {
    allowed_methods  = ["GET", "HEAD", "OPTIONS"]
    cached_methods   = ["GET", "HEAD", "OPTIONS"]
    target_origin_id = "S3-${aws_s3_bucket.main.id}"
    compress         = true

    response_headers_policy_id = aws_cloudfront_response_headers_policy.main.id

    forwarded_values {
      query_string = false

      cookies {
        forward = "none"
      }

      headers = [
        "Access-Control-Request-Headers",
        "Access-Control-Request-Method",
        "Origin"
      ]
    }

    viewer_protocol_policy = "https-only"
    min_ttl                = 0
    default_ttl            = 86400
    max_ttl                = 31536000
  }

  price_class = "PriceClass_All"

  restrictions {
    geo_restriction {
      restriction_type = "none"
    }
  }

  viewer_certificate {
    acm_certificate_arn      = data.aws_acm_certificate.main.arn
    minimum_protocol_version = "TLSv1.2_2021"
    ssl_support_method       = "sni-only"
  }
}
