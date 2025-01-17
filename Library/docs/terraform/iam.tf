data "aws_iam_policy_document" "main" {
  statement {
    sid    = "CloudFrontOrigin"
    effect = "Allow"
    actions = [
      "s3:GetObject"
    ]
    resources = [
      "arn:aws:s3:::${aws_s3_bucket.main.id}/*"
    ]
    principals {
      type        = "AWS"
      identifiers = [data.aws_cloudfront_origin_access_identity.main.iam_arn]
    }
  }
}
