variable "aws_region" {
  type = string
}

variable "aws_profile" {
  type    = string
  default = "default"
}

variable "project" {
  type = string
}

variable "project_shortname" {
  type = string
}

variable "environment" {
  type = string
}

variable "bucket_name" {
  type = string
}

variable "canonical_aws_public_zone_id" {
  type = string
}

variable "alias_aws_public_zone_id" {
  type    = string
  default = null
}

variable "aws_cloudfront_origin_access_identity_id" {
  type = string
}

variable "aws_wafv2_web_acl_arn" {
  type = string
}
