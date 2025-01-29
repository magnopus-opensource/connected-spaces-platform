data "aws_route53_zone" "canonical" {
  zone_id = var.canonical_aws_public_zone_id
}

data "aws_route53_zone" "alias" {
  count   = var.alias_aws_public_zone_id != null ? 1 : 0
  zone_id = var.alias_aws_public_zone_id
}

resource "aws_route53_record" "canonical" {

  alias {
    evaluate_target_health = "false"
    name                   = aws_cloudfront_distribution.main.domain_name
    zone_id                = aws_cloudfront_distribution.main.hosted_zone_id
  }

  name    = var.project_shortname
  type    = "A"
  zone_id = var.canonical_aws_public_zone_id
}

resource "aws_route53_record" "alias" {
  count = var.alias_aws_public_zone_id != null ? 1 : 0

  alias {
    evaluate_target_health = "false"
    name                   = aws_cloudfront_distribution.main.domain_name
    zone_id                = aws_cloudfront_distribution.main.hosted_zone_id
  }

  name    = data.aws_route53_zone.alias[0].name
  type    = "A"
  zone_id = var.alias_aws_public_zone_id
}
