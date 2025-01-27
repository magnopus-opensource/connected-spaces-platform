resource "aws_cloudfront_origin_access_control" "main" {
  name                              = var.bucket_name
  description                       = "Cloudfront Origin Access Control (${var.bucket_name})"
  origin_access_control_origin_type = "s3"
  signing_behavior                  = "always"
  signing_protocol                  = "sigv4"
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
    domain_name              = aws_s3_bucket.main.bucket_regional_domain_name
    origin_access_control_id = aws_cloudfront_origin_access_control.main.id
    origin_id                = "S3-${aws_s3_bucket.main.id}"
  }

  enabled             = true
  is_ipv6_enabled     = false # Using IPv4 only to mitigate VPN routing issues
  web_acl_id          = var.aws_wafv2_web_acl_arn
  default_root_object = "/index.html"

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

    viewer_protocol_policy = "redirect-to-https"
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

  dynamic "custom_error_response" {
    for_each = [403, 404]
    content {
      error_caching_min_ttl = 10
      error_code            = custom_error_response.value
      response_code         = 200
      response_page_path    = "/index.html"
    }
  }
}

resource "null_resource" "cloudfront_cache_invalidation" {
  provisioner "local-exec" {
    command = "aws cloudfront create-invalidation --distribution-id ${aws_cloudfront_distribution.main.id} --paths /'*'"
  }

  triggers = { for file in fileset(var.build_dir, "**/*") : file => filemd5("${var.build_dir}/${file}") }

  depends_on = [aws_s3_object.files]
}
