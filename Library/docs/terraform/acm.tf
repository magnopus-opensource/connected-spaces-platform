data "aws_acm_certificate" "main" {
  domain   = var.alias_aws_public_zone_id != null ? data.aws_route53_zone.alias.name : data.aws_route53_zone.canonical.name
  statuses = ["ISSUED"]
}
