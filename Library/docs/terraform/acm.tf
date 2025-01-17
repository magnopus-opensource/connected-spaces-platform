data "aws_acm_certificate" "main" {
  domain   = data.aws_route53_zone.canonical.name
  statuses = ["ISSUED"]
}
