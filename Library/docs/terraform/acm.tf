data "aws_acm_certificate" "main" {
  domain   = data.aws_route53_zone.alias.name
  statuses = ["ISSUED"]
}
