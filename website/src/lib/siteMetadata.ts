import type { Metadata } from "next";

const siteUrl = "https://atomik.tech";
const defaultImage = "/10-current-live-atomik-desk-v040a.png";

type PageMetadataInput = {
  title: string;
  description: string;
  path: string;
  openGraphTitle?: string;
  openGraphDescription?: string;
  twitterTitle?: string;
  twitterDescription?: string;
  image?: string;
  imageWidth?: number;
  imageHeight?: number;
};

function absoluteUrl(path: string) {
  if (path.startsWith("http://") || path.startsWith("https://")) return path;
  return `${siteUrl}${path.startsWith("/") ? path : `/${path}`}`;
}

export function pageMetadata({
  title,
  description,
  path,
  openGraphTitle,
  openGraphDescription,
  twitterTitle,
  twitterDescription,
  image = defaultImage,
  imageWidth = 1920,
  imageHeight = 1080,
}: PageMetadataInput): Metadata {
  const url = absoluteUrl(path);
  const imageUrl = absoluteUrl(image);
  const socialTitle = openGraphTitle || title;
  const socialDescription = openGraphDescription || description;

  return {
    title,
    description,
    alternates: { canonical: url },
    openGraph: {
      title: socialTitle,
      description: socialDescription,
      url,
      siteName: "ATOMiK",
      images: [{ url: imageUrl, width: imageWidth, height: imageHeight }],
      type: "website",
    },
    twitter: {
      card: "summary_large_image",
      title: twitterTitle || socialTitle,
      description: twitterDescription || socialDescription,
      images: [imageUrl],
    },
  };
}
